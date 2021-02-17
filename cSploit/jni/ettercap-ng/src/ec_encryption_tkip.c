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
#include <ec_inet.h>
#include <ec_checksum.h>
#include <ec_threads.h>

#include <openssl/rc4.h>

/* globals */

/* Note: copied from FreeBSD source code, RELENG 6, sys/net80211/ieee80211_crypto_tkip.c, 471 */
static const u_int16 Sbox[256] = {
   0xC6A5, 0xF884, 0xEE99, 0xF68D, 0xFF0D, 0xD6BD, 0xDEB1, 0x9154,
   0x6050, 0x0203, 0xCEA9, 0x567D, 0xE719, 0xB562, 0x4DE6, 0xEC9A,
   0x8F45, 0x1F9D, 0x8940, 0xFA87, 0xEF15, 0xB2EB, 0x8EC9, 0xFB0B,
   0x41EC, 0xB367, 0x5FFD, 0x45EA, 0x23BF, 0x53F7, 0xE496, 0x9B5B,
   0x75C2, 0xE11C, 0x3DAE, 0x4C6A, 0x6C5A, 0x7E41, 0xF502, 0x834F,
   0x685C, 0x51F4, 0xD134, 0xF908, 0xE293, 0xAB73, 0x6253, 0x2A3F,
   0x080C, 0x9552, 0x4665, 0x9D5E, 0x3028, 0x37A1, 0x0A0F, 0x2FB5,
   0x0E09, 0x2436, 0x1B9B, 0xDF3D, 0xCD26, 0x4E69, 0x7FCD, 0xEA9F,
   0x121B, 0x1D9E, 0x5874, 0x342E, 0x362D, 0xDCB2, 0xB4EE, 0x5BFB,
   0xA4F6, 0x764D, 0xB761, 0x7DCE, 0x527B, 0xDD3E, 0x5E71, 0x1397,
   0xA6F5, 0xB968, 0x0000, 0xC12C, 0x4060, 0xE31F, 0x79C8, 0xB6ED,
   0xD4BE, 0x8D46, 0x67D9, 0x724B, 0x94DE, 0x98D4, 0xB0E8, 0x854A,
   0xBB6B, 0xC52A, 0x4FE5, 0xED16, 0x86C5, 0x9AD7, 0x6655, 0x1194,
   0x8ACF, 0xE910, 0x0406, 0xFE81, 0xA0F0, 0x7844, 0x25BA, 0x4BE3,
   0xA2F3, 0x5DFE, 0x80C0, 0x058A, 0x3FAD, 0x21BC, 0x7048, 0xF104,
   0x63DF, 0x77C1, 0xAF75, 0x4263, 0x2030, 0xE51A, 0xFD0E, 0xBF6D,
   0x814C, 0x1814, 0x2635, 0xC32F, 0xBEE1, 0x35A2, 0x88CC, 0x2E39,
   0x9357, 0x55F2, 0xFC82, 0x7A47, 0xC8AC, 0xBAE7, 0x322B, 0xE695,
   0xC0A0, 0x1998, 0x9ED1, 0xA37F, 0x4466, 0x547E, 0x3BAB, 0x0B83,
   0x8CCA, 0xC729, 0x6BD3, 0x283C, 0xA779, 0xBCE2, 0x161D, 0xAD76,
   0xDB3B, 0x6456, 0x744E, 0x141E, 0x92DB, 0x0C0A, 0x486C, 0xB8E4,
   0x9F5D, 0xBD6E, 0x43EF, 0xC4A6, 0x39A8, 0x31A4, 0xD337, 0xF28B,
   0xD532, 0x8B43, 0x6E59, 0xDAB7, 0x018C, 0xB164, 0x9CD2, 0x49E0,
   0xD8B4, 0xACFA, 0xF307, 0xCF25, 0xCAAF, 0xF48E, 0x47E9, 0x1018,
   0x6FD5, 0xF088, 0x4A6F, 0x5C72, 0x3824, 0x57F1, 0x73C7, 0x9751,
   0xCB23, 0xA17C, 0xE89C, 0x3E21, 0x96DD, 0x61DC, 0x0D86, 0x0F85,
   0xE090, 0x7C42, 0x71C4, 0xCCAA, 0x90D8, 0x0605, 0xF701, 0x1C12,
   0xC2A3, 0x6A5F, 0xAEF9, 0x69D0, 0x1791, 0x9958, 0x3A27, 0x27B9,
   0xD938, 0xEB13, 0x2BB3, 0x2233, 0xD2BB, 0xA970, 0x0789, 0x33A7,
   0x2DB6, 0x3C22, 0x1592, 0xC920, 0x8749, 0xAAFF, 0x5078, 0xA57A,
   0x038F, 0x59F8, 0x0980, 0x1A17, 0x65DA, 0xD731, 0x84C6, 0xD0B8,
   0x82C3, 0x29B0, 0x5A77, 0x1E11, 0x7BCB, 0xA8FC, 0x6DD6, 0x2C3A,
};

#define TKIP_TA_OFFSET          10
#define TKIP_PHASE1_LOOP_COUNT  8
#define TKIP_TTAK_LEN           6
#define TKIP_WEP_128_KEY_LEN    16 /* 128 bit */

#define to_int16(hi, lo) ((u_int16)((lo) | (((u_int16)(hi)) << 8)))

#define RotR1(val) ((u_int16)(((val) >> 1) | ((val) << 15)))
#define Lo8(val)  ((u_int8)((val) & 0xff))
#define Hi8(val)  ((u_int8)((val) >> 8))
#define Lo16(val) ((u_int16)((val) & 0xffff))
#define Hi16(val) ((u_int16)((val) >> 16))
#define _S_(v) ((u_int16)(Sbox[Lo8(v)] ^ ((Sbox[Hi8(v)] << 8) | (Sbox[Hi8(v)] >> 8))))

#define pletohs(p)  ((u_int16)                              \
                    ((u_int16)*((const u_int8 *)(p)+1)<<8|  \
                    (u_int16)*((const u_int8 *)(p)+0)<<0))

#define Mk16_le(v) ((u_int16)pletohs(v))

/* protos */

int wpa_tkip_decrypt(u_char *mac, u_char *data, size_t len, struct wpa_sa sa);
static inline void get_TSC(u_int32 *TSC32, u_int16 *TSC16, u_char *data);
static inline void tkip_mixing_phase1(u_int16 *TTAK, u_int8 *TK, u_int8 *TA, u_int32 TSC32);
static inline void tkip_mixing_phase2(u_int8 *WEP, u_int8 *TK, u_int16 *TTAK, u_int16 TSC16);
static int tkip_decrypt(u_char *decbuf, size_t len, u_int8 *wep_seed);

/*******************************************/

/*
 * IEEE-802.11i-2004  8.3.2.1
 */
int wpa_tkip_decrypt(u_char *mac, u_char *data, size_t len, struct wpa_sa sa)
{
   u_int32 TSC32; /* TKIP Sequence Counter ( 2, 3, 4, 5 ) */
   u_int16 TSC16; /* TKIP Sequence Counter ( 1, 0 ) */
   u_int16 TTAK[TKIP_TTAK_LEN];
   u_int8  wep_seed[TKIP_WEP_128_KEY_LEN];
   u_char decbuf[len];

   if (len > UINT16_MAX) {
       return -ENOTHANDLED;
   }

   /* get the TKIP Sequence Counter */
   get_TSC(&TSC32, &TSC16, data);

   memset(TTAK, 0, sizeof(TTAK));

   /* create the seed for the WEP decryption */
   tkip_mixing_phase1(TTAK, sa.decryption_key, mac + TKIP_TA_OFFSET, TSC32);
   tkip_mixing_phase2(wep_seed, sa.decryption_key, TTAK, TSC16);

   /* copy the encrypted data to the decryption buffer (skipping the wpa parameters) */
   memcpy(decbuf, data + sizeof(struct wpa_header), len);

   /* decrypt the packet */
   if (tkip_decrypt(decbuf, len - WEP_CRC_LEN, wep_seed) != 0) {
      //DEBUG_MSG(D_VERBOSE, "WPA (TKIP) decryption failed, packet was skipped");
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
    * we have moved the data over the wpa header and the WEP crc was left
    * at the end of the packet.
    */
   memset(data + len - WEP_CRC_LEN, 0, WEP_CRC_LEN);

   return ESUCCESS;
}

/*
 * IEEE-802.11i-2004  8.3.2.2
 *
 * ------------------------------------------------
 * |TSC1|WEPSeed|TSC0|Reserved|TSC2|TSC3|TSC4|TSC5|
 * ------------------------------------------------
 */
static inline void get_TSC(u_int32 *TSC32, u_int16 *TSC16, u_char *data)
{

   *TSC16 = to_int16(data[0], data[2]);

   *TSC32 = ( (u_int32)(data[7]) << 24 ) |
            ( (u_int32)(data[6]) << 16 ) |
            ( (u_int32)(data[5]) << 8 ) |
            ( (u_int32)(data[4]) );

}

/*
 * IEEE-802.11i-2004  8.3.2.5
 */
static inline void tkip_mixing_phase1(u_int16 *TTAK, u_int8 *TK, u_int8 *TA, u_int32 TSC32)
{
   u_int16 i, j;

   /* Initialize the 80-bit TTAK from TSC (TSC32) and TA[0..5] */
   TTAK[0] = Lo16(TSC32);
   TTAK[1] = Hi16(TSC32);
   TTAK[2] = to_int16(TA[1], TA[0]);
   TTAK[3] = to_int16(TA[3], TA[2]);
   TTAK[4] = to_int16(TA[5], TA[4]);

   for (i = 0; i < TKIP_PHASE1_LOOP_COUNT; i++) {
      j = (u_int16)(2 * (i & 1));
      TTAK[0] = (u_int16)(TTAK[0] + _S_((u_int16)(TTAK[4] ^ to_int16(TK[1 + j],  TK[0 + j]))));
      TTAK[1] = (u_int16)(TTAK[1] + _S_((u_int16)(TTAK[0] ^ to_int16(TK[5 + j],  TK[4 + j]))));
      TTAK[2] = (u_int16)(TTAK[2] + _S_((u_int16)(TTAK[1] ^ to_int16(TK[9 + j],  TK[8 + j]))));
      TTAK[3] = (u_int16)(TTAK[3] + _S_((u_int16)(TTAK[2] ^ to_int16(TK[13 + j], TK[12 + j]))));
      TTAK[4] = (u_int16)(TTAK[4] + _S_((u_int16)(TTAK[3] ^ to_int16(TK[1 + j],  TK[0 + j]))) + i);
   }

}

static inline void tkip_mixing_phase2(u_int8 *WEP, u_int8 *TK, u_int16 *TTAK, u_int16 TSC16)
{
   int i;
   TTAK[5] = (u_int16)(TTAK[4] + TSC16);

   /* Step 2 - 96-bit bijective mixing using S-box */
   TTAK[0] = (u_int16)(TTAK[0] + _S_((u_int16)(TTAK[5] ^ Mk16_le(&TK[0]))));
   TTAK[1] = (u_int16)(TTAK[1] + _S_((u_int16)(TTAK[0] ^ Mk16_le(&TK[2]))));
   TTAK[2] = (u_int16)(TTAK[2] + _S_((u_int16)(TTAK[1] ^ Mk16_le(&TK[4]))));
   TTAK[3] = (u_int16)(TTAK[3] + _S_((u_int16)(TTAK[2] ^ Mk16_le(&TK[6]))));
   TTAK[4] = (u_int16)(TTAK[4] + _S_((u_int16)(TTAK[3] ^ Mk16_le(&TK[8]))));
   TTAK[5] = (u_int16)(TTAK[5] + _S_((u_int16)(TTAK[4] ^ Mk16_le(&TK[10]))));

   TTAK[0] = (u_int16)(TTAK[0] + RotR1((u_int16)(TTAK[5] ^ Mk16_le(&TK[12]))));
   TTAK[1] = (u_int16)(TTAK[1] + RotR1((u_int16)(TTAK[0] ^ Mk16_le(&TK[14]))));
   TTAK[2] = (u_int16)(TTAK[2] + RotR1(TTAK[1]));
   TTAK[3] = (u_int16)(TTAK[3] + RotR1(TTAK[2]));
   TTAK[4] = (u_int16)(TTAK[4] + RotR1(TTAK[3]));
   TTAK[5] = (u_int16)(TTAK[5] + RotR1(TTAK[4]));

   /*
    * Step 3 - bring in last of TK bits, assign 24-bit WEP IV value
    * WEP[0..2] is transmitted as WEP IV
    */
   WEP[0] = Hi8(TSC16);
   WEP[1] = (u_int8)((Hi8(TSC16) | 0x20) & 0x7F);
   WEP[2] = Lo8(TSC16);
   WEP[3] = Lo8((u_int16)((TTAK[5] ^ Mk16_le(&TK[0])) >> 1));

   for (i = 0; i < 6; i++) {
      WEP[4 + (2 * i)] = Lo8(TTAK[i]);
      WEP[5 + (2 * i)] = Hi8(TTAK[i]);
   }

}

/*
 * decryption of tkip is like WEP, RC4 algorithm
 */
static int tkip_decrypt(u_char *decbuf, size_t len, u_int8 *wep_seed)
{
   RC4_KEY key;

   /* initialize the RC4 key */
   RC4_set_key(&key, TKIP_WEP_128_KEY_LEN, wep_seed);

   /* decrypt the frame (len + 4 byte of crc) */
   RC4(&key, len + WEP_CRC_LEN, decbuf, decbuf);

   /*
    * check if the decryption was successful:
    * at the end of the packet there is a CRC check
    */
   if (CRC_checksum(decbuf, len + WEP_CRC_LEN, CRC_INIT) != CRC_RESULT) {
      //DEBUG_MSG(D_VERBOSE, "WEP: invalid key, the packet was skipped\n");
      return -ENOTHANDLED;
   }

   return ESUCCESS;
}

/* EOF */

// vim:ts=3:expandtab

