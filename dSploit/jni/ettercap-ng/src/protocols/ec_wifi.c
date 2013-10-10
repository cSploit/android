/*
    ettercap -- 802.11b (wifi) decoder module

    Copyright (C) ALoR & NaGA

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

    $Id: ec_wifi.c,v 1.29 2004/06/25 14:24:30 alor Exp $
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>
#include <ec_capture.h>
#include <ec_checksum.h>

#ifdef HAVE_OPENSSL
   #include <openssl/rc4.h>
   #include <openssl/md5.h>
#endif

/* globals */

struct wifi_header {
   u_int8   type;
      #define WIFI_DATA    0x08
      #define WIFI_BACON   0x80
      #define WIFI_ACK     0xd4
   u_int8   control;
      #define WIFI_STA_TO_STA 0x00  /* ad hoc mode */
      #define WIFI_STA_TO_AP  0x01
      #define WIFI_AP_TO_STA  0x02
      #define WIFI_AP_TO_AP   0x03
      #define WIFI_WEP        0x40
   u_int16  duration;
   /*
    * the following three fields has different meanings
    * depending on the control value... argh !!
    *
    *    - WIFI_STA_TO_STA  (ad hoc)
    *       ha1 -> dst 
    *       ha2 -> src
    *       ha3 -> bssid
    *    - WIFI_STA_TO_AP  
    *       ha1 -> bssid  
    *       ha2 -> src
    *       ha3 -> dst
    *    - WIFI_AP_TO_AP
    *       ha1 -> rx 
    *       ha2 -> tx
    *       ha3 -> dst
    *       ha4 -> src
    *    - WIFI_AP_TO_STA
    *       ha1 -> dst
    *       ha2 -> bssid
    *       ha3 -> src
    */    
   u_int8   ha1[ETH_ADDR_LEN];
   u_int8   ha2[ETH_ADDR_LEN];
   u_int8   ha3[ETH_ADDR_LEN];
   u_int16  seq;
   /* this field is present only if control is WIFI_AP_TO_AP */
   /* u_int8   ha3[ETH_ADDR_LEN]; */
};

struct llc_header {
   u_int8   dsap;
   u_int8   ssap;
   u_int8   control;
   u_int8   org_code[3];
   u_int16  proto;
};

/*
 * WEP is based on RSA's rc4 stream cipher and uses a 24-bit initialization
 * vector (iv), which is concatenated with a 40-bit or 104-bit secret shared key
 * to create a 64-bit or 128-bit key which is used as the rc4 seed. Most cards
 * either generate the 24-bit iv using a counter or by using some sort of pseudo
 * random number generator (prng). The payload is then encrypted along with an
 * appended 32-bit checksum and sent out with the iv in plaintext as illustrated:
 * 
 *    0                   1                   2                   3
 *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |                                                               |
 *   |                         802.11 Header                         |
 *   |                                                               |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |     IV[0]     |     IV[1]     |     IV[2]     |    Key ID     |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   | . . . . . . SNAP[0] . . . . . | . . . . . SNAP[1] . . . . . . |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   | . . . . . . SNAP[2] . . . . . | . . . . Protocol ID . . . . . |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   | . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . |
 *   | . . . . . . . . . . . . . Payload . . . . . . . . . . . . . . |
 *   | . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   | . . . . . . . . . . . 32-bit Checksum . . . . . . . . . . . . |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * 
 *   . - denotes encrypted portion of packet
 * 
 * After the data is sent out, the receiver simply concatenates the received iv
 * with their secret key to decrypt the payload. If the checksum checks out, then
 * the packet is valid.
 */

#define IV_LEN 3

struct wep_header {
   u_int8   init_vector[IV_LEN];
   u_int8   key;
};

/* encapsulated ethernet */
static u_int8 WIFI_ORG_CODE[3] = {0x00, 0x00, 0x00};

#define WEP_KEY_MAX_LEN    32 /* 256 bit */

static u_int8 wkey[WEP_KEY_MAX_LEN];  
static size_t wlen;
 

/* protos */

FUNC_DECODER(decode_wifi);
FUNC_ALIGNER(align_wifi);
void wifi_init(void);
static int wep_decrypt(u_char *buf, size_t len);
int set_wep_key(u_char *string);
static void make_key_64(u_char *string);
static void make_key_128(u_char *string);

/*******************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init wifi_init(void)
{
   add_decoder(LINK_LAYER, IL_TYPE_WIFI, decode_wifi);
   add_aligner(IL_TYPE_WIFI, align_wifi);
}


FUNC_DECODER(decode_wifi)
{
   struct wifi_header *wifi;
   struct llc_header *llc;
   struct wep_header *wep;
   FUNC_DECODER_PTR(next_decoder) = NULL;

   DECODED_LEN = sizeof(struct wifi_header);
      
   wifi = (struct wifi_header *)DECODE_DATA;
   
   /* we are interested only in wifi data packets */
   if (wifi->type != WIFI_DATA) {
      return NULL;
   }

   /* 
    * capture only "complete" and not retransmitted packets 
    * we don't want to deal with fragments (0x04) or retransmission (0x08) 
    */
   switch (wifi->control & 0x03) {
      
      case WIFI_STA_TO_STA:
         memcpy(PACKET->L2.src, wifi->ha2, ETH_ADDR_LEN);
         memcpy(PACKET->L2.dst, wifi->ha1, ETH_ADDR_LEN);
         break;
   
   
      case WIFI_STA_TO_AP:
         memcpy(PACKET->L2.src, wifi->ha2, ETH_ADDR_LEN);
         memcpy(PACKET->L2.dst, wifi->ha3, ETH_ADDR_LEN);
         break;
   
      case WIFI_AP_TO_STA:
         memcpy(PACKET->L2.src, wifi->ha3, ETH_ADDR_LEN);
         memcpy(PACKET->L2.dst, wifi->ha1, ETH_ADDR_LEN);
         break;
         
      case WIFI_AP_TO_AP:
         /* 
          * XXX - fix this or ignore this case...
          *
          * SHIT !! we have alignment problems here...
          */
#if 0         
         /* 
          * get the logical link layer header 
          * there is one more field (ha4) in this case
          */
         llc = (struct llc_header *)((u_char *)wifi + sizeof(struct wifi_header) + ETH_ADDR_LEN);
         DECODED_LEN += sizeof(struct llc_header) + ETH_ADDR_LEN;
         
         memcpy(PACKET->L2.src, (char *)(wifi + 1), ETH_ADDR_LEN);
         memcpy(PACKET->L2.dst, wifi->ha3, ETH_ADDR_LEN);
#endif
         return NULL;
         break;
      
      default:
         return NULL;
   }
  
   /* the frame is crypted with WEP */
   if (wifi->control & WIFI_WEP) {
      
      /* get the WEP header */
      wep = (struct wep_header *)(wifi + 1);
      DECODED_LEN += sizeof(struct wep_header);

      /* decrypt the packet */
      if (wep_decrypt((u_char *)wep, DECODE_DATALEN - DECODED_LEN) != ESUCCESS)
         return NULL;
     
      /* the wep header was overwritten, remove it from the decoded portion */
      DECODED_LEN -= sizeof(struct wep_header);

      /* remove the WEP bit from the header since the data are now decrypted */
      wifi->control &= ~WIFI_WEP;
   } 
   
   /* get the logical link layer header */
   llc = (struct llc_header *)(wifi + 1);
   DECODED_LEN += sizeof(struct llc_header);
   
   /* org_code != encapsulated ethernet not yet supported */
   if (memcmp(llc->org_code, WIFI_ORG_CODE, 3))
      return NULL;
      
   /* fill the packet object with sensitive data */
   PACKET->L2.header = (u_char *)DECODE_DATA;
   PACKET->L2.proto = IL_TYPE_WIFI;
   PACKET->L2.len = DECODED_LEN;

   /* HOOK POINT: HOOK_PACKET_WIFI */
   hook_point(HOOK_PACKET_WIFI, po);
   
   /* leave the control to the next decoder */
   next_decoder = get_decoder(NET_LAYER, ntohs(llc->proto));
   EXECUTE_DECODER(next_decoder);
  
   /* no modification to wifi header should be done */
   
   return NULL;
}

/*
 * alignment function
 */
FUNC_ALIGNER(align_wifi)
{
   /* already aligned */
   return (32 - sizeof(struct wifi_header) - sizeof(struct llc_header));
}


/*
 * WEP decrypt function
 */
static int wep_decrypt(u_char *buf, size_t len)
{
#ifdef HAVE_OPENSSL
   RC4_KEY key;
   u_char seed[32]; /* 256 bit for the wep key */
   struct wep_header *wep;
   u_char *encbuf;
   u_char decbuf[len];

   /* the key was not set, don't try to decript it */
   if (wlen == 0)
      return -ENOTHANDLED;

   /* get the wep header */
   wep = (struct wep_header *)buf;
   len -= sizeof(struct wep_header);

   /* get the key index */
   wep->key >>= 6;

   /* sanity check on the key index */
   if (wep->key * 5 > (int)(WEP_KEY_MAX_LEN - wlen)) {
      DISSECT_MSG("WEP: invalid key index, the packet was skipped\n");
      return -ENOTHANDLED;
   }
   
   encbuf = (u_char *)(wep + 1);
   
   /* copy the IV in the first 24 bit of the RC4 seed */
   memcpy(seed, wep->init_vector, IV_LEN);

   /* 
    * complete the seed with x bit from the secret key 
    * 
    * when using 64 bit WEP, the four keys are stored
    * in the wkey array, every 5 bytes there is a new
    * key, so we can indicize them with wep->key * 5
    */
   memcpy(seed + IV_LEN, &wkey[wep->key * 5], wlen);
   
   /* initialize the RC4 key */
   RC4_set_key(&key, IV_LEN + wlen, seed);
  
   /* decrypt the frame (len + 4 byte of crc) */
   RC4(&key, len + 4, encbuf, decbuf);
   
   /* 
    * check if the decryption was successfull:
    * at the end of the packet there is a CRC check
    */
   if (CRC_checksum(decbuf, len + 4, CRC_INIT) != CRC_RESULT) {
      DISSECT_MSG("WEP: invalid key, the packet was skipped\n");
      return -ENOTHANDLED;
   } 
  
   /* 
    * copy the decrypted packet over the original one
    * overwriting the wep header. this way the packet is
    * identical to a non-WEP one.
    */
   memcpy(buf, decbuf, len);

   /* 
    * wipe out the remaining bytes at the end of the packets
    * we have moved the data over the wep header and the crc was left
    * at the end of the packet.
    */
   memset(buf + len, 0, sizeof(struct wep_header) + sizeof(u_int32));
   
   return ESUCCESS;
#else
   return -EFATAL;
#endif
}

/*
 * check if the string is ok to be used as a key
 * for WEP.
 * the format is:  n:string  where n is the number of bit used 
 * for the RC4 seed. you can use strings or hex values.
 * for example:
 *    64:s:alor1
 *    64:p:naga
 *    128:s:ettercapng070
 *    64:s:\x01\x02\x03\x04\x05
 */
int set_wep_key(u_char *string)
{
   int bit = 0, i;
   u_char *p, type;
   char *tok;
   char s[strlen(string) + 1];
   
   DEBUG_MSG("set_wep_key: %s", string);
   
   memset(wkey, 0, sizeof(wkey));
   strcpy(s, string);

   p = ec_strtok(s, ":", &tok);
   if (p == NULL)
      SEMIFATAL_ERROR("Invalid parsing of the WEP key");

   bit = atoi(p);

   /* sanity check */
   if (bit <= 0)
      SEMIFATAL_ERROR("Unsupported WEP key lenght");

   /* the len of the secret part of the RC4 seed */
   wlen = bit / 8 - IV_LEN;
   
   /* sanity check */
   if (wlen > sizeof(wkey))
      SEMIFATAL_ERROR("Unsupported WEP key lenght");
  
   if (bit != 64 && bit != 128)
      SEMIFATAL_ERROR("Unsupported WEP key lenght");

   /* get the type of the key */
   p = ec_strtok(NULL, ":", &tok);
   if (p == NULL)
      SEMIFATAL_ERROR("Invalid parsing of the WEP key");
  
   type = *p;
   
   /* get the third part of the string */
   p = ec_strtok(NULL, ":", &tok);
   if (p == NULL)
      SEMIFATAL_ERROR("Invalid parsing of the WEP key");
   
   if (type == 's') {
      /* escape the string and check its lenght */
      if (strescape(wkey, p) != (int)wlen)
         SEMIFATAL_ERROR("Specified WEP key lenght does not match the given string");
   } else if (type == 'p') {
      /* create the key from the passphrase */
      if (bit == 64)
         make_key_64(p);
      else if (bit == 128)
         make_key_128(p);
         
   } else {
      SEMIFATAL_ERROR("Invalid parsing of the WEP key");
   }
 
   USER_MSG("Using WEP key: ");
   for (i = 0; i < (int)wlen; i++)
      USER_MSG("%02x", wkey[i]);
   USER_MSG("\n");
   
   return ESUCCESS;
}

/*
 * generate a key set (4 keys) from a passfrase
 */
static void make_key_64(u_char *string)
{
   int i, seed = 0;
        
   /*
    * seed is generated by xor'ing the keystring bytes
    * into the four bytes of the seed, starting at the little end
    */     
   for(i = 0; string[i]; i++) {
      seed ^= (string[i] << ((i & 0x03) * 8));
   }

   /* generate the 4 keys from the seed */
   for(i = 0; i < 5*4; i++) {
      seed *= 0x000343fd;
      seed += 0x00269ec3;
      wkey[i] = seed >> 16;
   }
    
}

static void make_key_128(u_char *string)
{
#ifdef HAVE_OPENSSL
   MD5_CTX ctx;
   u_char buf[64];
   u_char digest[MD5_DIGEST_LENGTH];
   int i, j = 0;

   /* repeat the string until buf is full */
   for (i = 0; i < 64; i++) {
      if(string[j] == 0)
         j = 0;
      buf[i] = string[j++];
   }

   /* compute the md5 digest of the buffer */
   MD5_Init(&ctx);
   MD5_Update(&ctx, buf, sizeof buf);
   MD5_Final(digest, &ctx);
                      
   /* 
    * copy the digest into the key 
    * 13 byte == 104 bit
    */
   memset(wkey, 0, WEP_KEY_MAX_LEN);
   memcpy(wkey, digest, 13);
#endif
}

/* EOF */

// vim:ts=3:expandtab

