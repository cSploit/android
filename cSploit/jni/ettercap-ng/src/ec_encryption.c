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

#include <openssl/rc4.h>
#include <openssl/md5.h>
#include <openssl/x509.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/aes.h>

/* globals */

static LIST_HEAD(, wpa_session) wpa_sess_root;
static pthread_mutex_t root_mutex = PTHREAD_MUTEX_INITIALIZER;

/* protos */

int wifi_key_prepare(char *key_string);

int wep_decrypt(u_char *buf, size_t len, u_char *wkey, size_t wlen);
static int set_wep_key(char *string);
static void make_key_64(u_char *string, u_char *key);
static void make_key_128(u_char *string, u_char *key);

static int set_wpa_key(char *string);
void wpa_sess_add(u_char *sta, struct wpa_sa *sa);
void wpa_sess_del(u_char *sta);
int wpa_sess_get(u_char *sta, struct wpa_sa *sa);
int wpa_generate_PTK(u_char *bssid, u_char *sta, u_char *pmk, u_char *snonce, u_char *anonce,u_int16 bits, u_char *kck);
int wpa_check_MIC(struct eapol_header *eapol, struct eapol_key_header* eapol_key, size_t eapol_len, u_char *kck, int algo);
int wpa_decrypt_broadcast_key(struct eapol_key_header *eapol_key, struct rsn_ie_header *rsn_ie, struct wpa_sa *sa);
int wpa_decrypt(u_char *mac, u_char *data, size_t len, struct wpa_sa sa);
extern int wpa_ccmp_decrypt(u_char *mac, u_char *data, size_t len, struct wpa_sa sa);
extern int wpa_tkip_decrypt(u_char *mac, u_char *data, size_t len, struct wpa_sa sa);

/*******************************************/

/*
 * WEP decrypt function
 */
int wep_decrypt(u_char *buf, size_t len, u_char *wkey, size_t wlen)
{
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
   if (wep->key * 5 > (int)(MAX_WKEY_LEN - wlen)) {
      //DEBUG_MSG(D_VERBOSE, "WEP: invalid key index, the packet was skipped");
      return -ENOTHANDLED;
   }

   encbuf = (u_char *)(wep + 1);

   /* copy the IV in the first 24 bit of the RC4 seed */
   memcpy(seed, wep->init_vector, WEP_IV_LEN);

   /*
    * complete the seed with x bit from the secret key
    *
    * when using 64 bit WEP, the four keys are stored
    * in the wkey array, every 5 bytes there is a new
    * key, so we can indicize them with wep->key * 5
    */
   memcpy(seed + WEP_IV_LEN, &wkey[wep->key * 5], wlen);

   /* initialize the RC4 key */
   RC4_set_key(&key, WEP_IV_LEN + wlen, seed);

   /* decrypt the frame (len + 4 byte of crc) */
   RC4(&key, len + WEP_CRC_LEN, encbuf, decbuf);

   /*
    * check if the decryption was successful:
    * at the end of the packet there is a CRC check
    */
   if (CRC_checksum(decbuf, len + WEP_CRC_LEN, CRC_INIT) != CRC_RESULT) {
      //DEBUG_MSG(D_VERBOSE, "WEP decryption failed, the packet was skipped");
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
   memset(buf + len, 0, WEP_CRC_LEN);

   return ESUCCESS;
}

/*
 * parse the string provided by the user and set the internally used buffer
 * the format is:
 *    type:bits:t:string
 * where:
 *    type: can be wep, wpa-psw, wpa-psk
 *    bits: is the number of bits used for the key
 *    t: can be:
 *       s: for strings (or hexadecimal escaped values)
 *       p: for passwords that will be used to generate the key
 * for example:
 *    wep:64:p:ciao
 *    wep:64:s:alor1
 *    wep:128:s:rcsredirect12
 *    wep:64:s:\x01\x02\x03\x04\x05
 *    wpa:pwd:password:ssid
 *    wpa:psk:663eb260e87cf389c6bd7331b28d82f5203b0cae4e315f9cbb7602f3236708a6
 */

int wifi_key_prepare(char *key_string)
{
   int status = -EINVALID;
   char *ks;
   char *p;

   if (key_string == NULL)
      return -EINVALID;

   ks = strdup(key_string);

   if ((p = strchr(ks, ':')) != NULL)
      *p = 0;

   /* the following string is a definition for WEP */
   if (!strcasecmp(ks, "wep")) {
      GBL_WIFI->wifi_schema = WIFI_WEP;
      status = set_wep_key(p + 1);
   }

   /* the following string is a definition for WPA */
   if (!strcasecmp(ks, "wpa")) {
      GBL_WIFI->wifi_schema = WIFI_WPA;
      status = set_wpa_key(p + 1);
   }

   SAFE_FREE(ks);
   return status;
}


int set_wep_key(char *string)
{
   int bit = 0;
   char *p, type;
   char *tok;
   char s[strlen(string) + 1];
   u_char tmp_wkey[512];
   size_t tmp_wkey_len;
   char tmp[128];

   memset(GBL_WIFI->wkey, 0, sizeof(GBL_WIFI->wkey));
   GBL_WIFI->wkey_len = 0;

   strcpy(s, string);

   p = ec_strtok(s, ":", &tok);
   if (p == NULL)
      SEMIFATAL_ERROR("Invalid parsing of the WEP key");

   bit = atoi(p);

   /* sanity check */
   if (bit <= 0)
      SEMIFATAL_ERROR("Unsupported WEP key lenght");

   /* the len of the secret part of the RC4 seed */
   tmp_wkey_len = bit / 8 - WEP_IV_LEN;

   /* sanity check */
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
      if (strescape((char *)tmp_wkey, p) != (int)tmp_wkey_len)
    	  SEMIFATAL_ERROR("Specified WEP key length does not match the given string");
   } else if (type == 'p') {
      /* create the key from the passphrase */
      if (bit == 64)
         make_key_64((u_char *)p, tmp_wkey);
      else if (bit == 128)
         make_key_128((u_char *)p, tmp_wkey);

   } else {
     SEMIFATAL_ERROR("Invalid parsing of the WEP key");
   }

   /* print the final string */
   USER_MSG("Using WEP key: %s\n", str_tohex(tmp_wkey, tmp_wkey_len, tmp, sizeof(tmp)));

   memcpy(GBL_WIFI->wkey, tmp_wkey, sizeof(GBL_WIFI->wkey));
   GBL_WIFI->wkey_len = tmp_wkey_len;

   return ESUCCESS;
}

/*
 * generate a key set (4 keys) from a passfrase
 */
static void make_key_64(u_char *string, u_char *key)
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
      key[i] = seed >> 16;
   }

}

static void make_key_128(u_char *string, u_char *key)
{
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
   memset(key, 0, MAX_WKEY_LEN);
   memcpy(key, digest, 13);
}


static int set_wpa_key(char *string)
{
   char *p;
   char *pass;
   char *ssid;
   char tmp[128];
   int i;

   /* we need to generate the key */
   if (!strncasecmp(string, "pwd", 3)) {
      if ((p = strchr(string + + strlen("pwd") + 1, ':')) != NULL) {
         *p = 0;
      } else {
    	 SEMIFATAL_ERROR("Invalid parsing of the WPA password (missing SSID)");
      }

      /* the len of the password */
      i = strlen(string + strlen("pwd") + 1);

      /* sanity check */
      if (i < 8 || i > 63) {
         SEMIFATAL_ERROR("Invalid parsing of the WPA-PWD password (must be 8..63 chars)");
      }

      SAFE_STRDUP(pass, string + strlen("pwd") + 1);
      SAFE_STRDUP(ssid, p + 1);

      /*
       * undocumented function from OPENSSL which implement the PBKDF2 function used to generate the passphrase
       * the 4096 number of iterations was taken from wpa_passphrase.c of wpa_supplicant package
       */
      PKCS5_PBKDF2_HMAC_SHA1(pass, strlen(pass), (u_char *)ssid, strlen(ssid), 4096, 32, GBL_WIFI->wkey);

      SAFE_FREE(pass);
      SAFE_FREE(ssid);
   }

   /* just take the key and store it */
   if (!strncasecmp(string, "psk", 3)) {
      /* the hex string should be 32 bytes in hex format */
      if (strlen(string + strlen("psk") + 1) != 64) {
         SEMIFATAL_ERROR("Invalid parsing of the WPA-PSK password (must be 64 chars)");
      }
      /* parse the hex string into bytes */
      str_hex_to_bytes(string + strlen("psk") + 1, GBL_WIFI->wkey);
   }

   /* print the final string */
   USER_MSG("Using WPA key: %s\n", str_tohex(GBL_WIFI->wkey, WPA_KEY_LEN, tmp, sizeof(tmp)));

   return ESUCCESS;
}



void wpa_sess_add(u_char *sta, struct wpa_sa *sa)
{
   struct wpa_session *e, *s;
   char tmp[MAX_ASCII_ADDR_LEN];

   /* alloc the new element */
   SAFE_CALLOC(e, 1, sizeof(struct wpa_session));

   if (sta)
      memcpy(&e->sta, sta, ETH_ADDR_LEN);

   if (sa) {
      /* get the time of the creation.
       * this will be used to timeout the entry if we miss some packets
       * to prevent inconsistent state forever
       */
      gettimeofday(&sa->tv, NULL);

      memcpy(&e->sa, sa, sizeof(struct wpa_sa));
   }
   /* insert it in the list */
   pthread_mutex_lock(&root_mutex);

   /* check if the session already exists */
   LIST_FOREACH(s, &wpa_sess_root, next) {
      if (!memcmp(&e->sta, &s->sta, ETH_ADDR_LEN)) {
         /* already present in the list, replace the SA */
         if (sa) {
            memcpy(&s->sa, sa, sizeof(struct wpa_sa));
            /* update the time value */
            gettimeofday(&s->sa.tv, NULL);
         }

         USER_MSG("WPA session updated for [%s]\n", mac_addr_ntoa(e->sta, tmp));

         pthread_mutex_unlock(&root_mutex);
         return;
      }
   }

   LIST_INSERT_HEAD(&wpa_sess_root, e, next);
   pthread_mutex_unlock(&root_mutex);

   USER_MSG("New WPA session for [%s]\n", mac_addr_ntoa(e->sta, tmp));
}


void wpa_sess_del(u_char *sta)
{
   struct wpa_session *e, *tmp;
   char tmac[MAX_ASCII_ADDR_LEN];

   pthread_mutex_lock(&root_mutex);
   LIST_FOREACH_SAFE(e, &wpa_sess_root, next, tmp) {
      if (!memcmp(&e->sta, sta, ETH_ADDR_LEN)) {
         LIST_REMOVE(e, next);
         USER_MSG("WPA session deleted for [%s]\n", mac_addr_ntoa(e->sta, tmac));
         SAFE_FREE(e);
         break;
      }
   }
   pthread_mutex_unlock(&root_mutex);
}


int wpa_sess_get(u_char *sta, struct wpa_sa *sa)
{
   struct wpa_session *e;

   pthread_mutex_lock(&root_mutex);
   LIST_FOREACH(e, &wpa_sess_root, next) {
      if (!memcmp(&e->sta, sta, ETH_ADDR_LEN)) {
         memcpy(sa, &e->sa, sizeof(struct wpa_sa));
         pthread_mutex_unlock(&root_mutex);
         return ESUCCESS;
      }
   }
   pthread_mutex_unlock(&root_mutex);

   return -ENOTFOUND;
}

/* Function used to derive the PTK. Refer to IEEE 802.11I-2004, 8.5.1 */

/* derive the PTK from the BSSID, STA MAC, PMK (WPA-PSK), SNonce, ANonce */
int wpa_generate_PTK(u_char *bssid, u_char *sta, u_char *pmk, u_char *snonce, u_char *anonce, u_int16 bits, u_char *kck)
{
   u_int8 i;
   u_int len;
   u_char buff[100];
   size_t offset = sizeof("Pairwise key expansion");

   memset(buff, 0, 100);

   /* initialize the buffer */
   memcpy(buff, "Pairwise key expansion", offset);

   /*   Min(AA, SPA) || Max(AA, SPA)  */
   if (memcmp(sta, bssid, ETH_ADDR_LEN) < 0) {
      memcpy(buff + offset, sta, ETH_ADDR_LEN);
      memcpy(buff + offset + ETH_ADDR_LEN, bssid, ETH_ADDR_LEN);
   } else {
      memcpy(buff + offset, bssid, ETH_ADDR_LEN);
      memcpy(buff + offset + ETH_ADDR_LEN, sta, ETH_ADDR_LEN);
   }

   /* move after AA SPA */
   offset += ETH_ADDR_LEN * 2;

   /*   Min(ANonce,SNonce) || Max(ANonce,SNonce)  */
   if (memcmp(snonce, anonce, WPA_NONCE_LEN) < 0 ) {
      memcpy(buff + offset, snonce, WPA_NONCE_LEN);
      memcpy(buff + offset + WPA_NONCE_LEN, anonce, WPA_NONCE_LEN);
   } else {
      memcpy(buff + offset, anonce, WPA_NONCE_LEN);
      memcpy(buff + offset + WPA_NONCE_LEN, snonce, WPA_NONCE_LEN);
   }

   /* move after ANonce SNonce */
   offset += WPA_NONCE_LEN * 2;

   memset(kck, 0, WPA_PTK_LEN);

   /* generate the PTK */
   for (i = 0; i < (bits + 159)/160; i++) {
       buff[offset] = i;

       /* the buffer (ptk) is large enough (see declaration) */
       HMAC(EVP_sha1(), pmk, WPA_KEY_LEN, buff, 100, kck + i * 20, &len);
   }

   return ESUCCESS;
}


int wpa_check_MIC(struct eapol_header *eapol, struct eapol_key_header* eapol_key, size_t eapol_len, u_char *kck, int algo)
{
   u_char mic[WPA_MICKEY_LEN];
   u_int len;
   u_char hmac_mic[20]; /* MIC 16 byte, the HMAC-SHA1 use a buffer of 20 bytes */

   /* copy the MIC from the EAPOL packet */
   memcpy(mic, eapol_key->key_MIC, WPA_MICKEY_LEN);

   /* set to 0 the MIC in the EAPOL packet (to calculate the MIC) */
   memset(eapol_key->key_MIC, 0, WPA_MICKEY_LEN);

   if (algo == WPA_KEY_TKIP) {
       /* use HMAC-MD5 for the EAPOL-Key MIC   */
       HMAC(EVP_md5(), kck, WPA_KCK_LEN, (u_char *)eapol, eapol_len, hmac_mic, &len);
   } else if (algo == WPA_KEY_CCMP) {
       /* use HMAC-SHA1-128 for the EAPOL-Key MIC */
       HMAC(EVP_sha1(), kck, WPA_KCK_LEN, (u_char *)eapol, eapol_len, hmac_mic, &len);
   } else
       /* key descriptor version not recognized */
       return -EINVALID;

   /* restore the MIC in the EAPOL packet */
   memcpy(eapol_key->key_MIC, mic, WPA_MICKEY_LEN);

   /* compare calculated MIC with the Key MIC and return result (0 means success) */
   return memcmp(mic, hmac_mic, WPA_MICKEY_LEN);
}


int wpa_decrypt_broadcast_key(struct eapol_key_header *eapol_key, struct rsn_ie_header *rsn_ie, struct wpa_sa *sa)
{
   //guint8  new_key[32];
   u_int8  *encrypted_key;
   u_int16 key_len = 0;
   //static AIRPDCAP_KEY_ITEM dummy_key; /* needed in case AirPDcapRsnaMng() wants the key structure */

char tmp[512];

   /* Preparation for decrypting the group key - determine group key data length */
   /* depending on whether it's a TKIP or AES encryption key */
   if (sa->algo == WPA_KEY_TKIP) {
      key_len = ntohs(eapol_key->key_len);
   } else if (sa->algo == WPA_KEY_CCMP){
      key_len = ntohs(eapol_key->key_data_len);
   }

   /* sanity check */
   if (key_len > sizeof(struct rsn_ie_header) || key_len == 0)
      return -ENOTHANDLED;

   /* Encrypted key is in the information element field of the EAPOL key packet */
   SAFE_CALLOC(encrypted_key, key_len, sizeof(u_int8));

   DEBUG_MSG("Encrypted Broadcast key:", str_tohex(encrypted_key, key_len, tmp, sizeof(tmp)));
   DEBUG_MSG("KeyIV:", str_tohex(eapol_key->key_IV, 16, tmp, sizeof(tmp)));
   DEBUG_MSG("decryption_key:", str_tohex(sa->ptk + 16, 16, tmp, sizeof(tmp)));

   /*
    * XXX - implement broadcast key
    * we don't really need it, it is used only for multicast and broadcast packets
    */

#if 0
    /* Build the full decryption key based on the IV and part of the pairwise key */
    memcpy(new_key, pEAPKey->key_iv, 16);
     memcpy(new_key+16, decryption_key, 16);
     DEBUG_DUMP("FullDecrKey:", new_key, 32);

     if (key_version == AIRPDCAP_WPA_KEY_VER_NOT_CCMP){
        guint8 dummy[256];
        /* TKIP key */
        /* Per 802.11i, Draft 3.0 spec, section 8.5.2, p. 97, line 4-8, */
        /* group key is decrypted using RC4.  Concatenate the IV with the 16 byte EK (PTK+16) to get the decryption key */

        rc4_state_struct rc4_state;
        crypt_rc4_init(&rc4_state, new_key, sizeof(new_key));

        /* Do dummy 256 iterations of the RC4 algorithm (per 802.11i, Draft 3.0, p. 97 line 6) */
        crypt_rc4(&rc4_state, dummy, 256);
        crypt_rc4(&rc4_state, encrypted_key, key_len);

     } else if (key_version == AIRPDCAP_WPA_KEY_VER_AES_CCMP){
        /* AES CCMP key */

        guint8 key_found;
        guint16 key_index;
        guint8 *decrypted_data;

        /* This storage is needed for the AES_unwrap function */
        decrypted_data = (guint8 *) g_malloc(key_len);

        AES_unwrap(decryption_key, 16, encrypted_key,  key_len, decrypted_data);

        /* With WPA2 what we get after Broadcast Key decryption is an actual RSN structure.
           The key itself is stored as a GTK KDE
           WPA2 IE (1 byte) id = 0xdd, length (1 byte), GTK OUI (4 bytes), key index (1 byte) and 1 reserved byte. Thus we have to
           pass pointer to the actual key with 8 bytes offset */

        key_found = FALSE;
        key_index = 0;
        while(key_index < key_len && !key_found){
           guint8 rsn_id;

           /* Get RSN ID */
           rsn_id = decrypted_data[key_index];

           if (rsn_id != 0xdd){
              key_index += decrypted_data[key_index+1]+2;
           }else{
              key_found = TRUE;
           }
        }

        if (key_found){
           /* Skip over the GTK header info, and don't copy past the end of the encrypted data */
           memcpy(encrypted_key, decrypted_data+key_index+8, key_len-key_index-8);
        }

        g_free(decrypted_data);
     }

     /* Decrypted key is now in szEncryptedKey with len of key_len */
     DEBUG_DUMP("Broadcast key:", encrypted_key, key_len);

     /* Load the proper key material info into the SA */
     sa->key = &dummy_key;
     sa->validKey = TRUE;
     sa->wpa.key_ver = key_version;
     memset(sa->wpa.ptk, 0, sizeof(sa->wpa.ptk));
     memcpy(sa->wpa.ptk+32, szEncryptedKey, key_len);
     g_free(szEncryptedKey);

#endif

   SAFE_FREE(encrypted_key);

   return ESUCCESS;
}


int wpa_decrypt(u_char *mac, u_char *data, size_t len, struct wpa_sa sa)
{
   /*
    * TKIP - for wpa
    * CCMP - for wpa2
    */

   if (sa.algo == WPA_KEY_CCMP) {
      return wpa_ccmp_decrypt(mac, data, len, sa);
   } else if (sa.algo == WPA_KEY_TKIP) {
      return wpa_tkip_decrypt(mac, data, len, sa);
   }

   /* not reached */
   return -ENOTHANDLED;
}

/* EOF */

// vim:ts=3:expandtab

