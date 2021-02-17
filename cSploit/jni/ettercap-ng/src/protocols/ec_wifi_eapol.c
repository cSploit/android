/*
    ettercap -- EAP On Line 802.1x authentication packets for wifi

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
#include <ec_decode.h>
#include <ec_capture.h>
#include <ec_encryption.h>

/* globals */

#define EAPOL_4WAY_MESSAGE_ZERO  0
#define EAPOL_4WAY_MESSAGE_ONE   1
#define EAPOL_4WAY_MESSAGE_TWO   2
#define EAPOL_4WAY_MESSAGE_THREE 3
#define EAPOL_4WAY_MESSAGE_FOUR  4
#define EAPOL_4WAY_MESSAGE_GROUP 5

/* protos */

FUNC_DECODER(decode_eapol);
void eapol_init(void);
static int eapol_4way_handshake(struct eapol_key_header *eapol_key);
static int eapol_enc_algo(struct eapol_key_header *eapol_key);

/*******************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init eapol_init(void)
{
   add_decoder(NET_LAYER, LL_TYPE_8021x, decode_eapol);
}


FUNC_DECODER(decode_eapol)
{
   struct eapol_header *eapol;
   struct eapol_key_header *eapol_key;
   struct rsn_ie_header *rsn_ie;
   char sta_address[ETH_ASCII_ADDR_LEN];
   u_char sta[ETH_ADDR_LEN], bssid[ETH_ADDR_LEN];
   int message, algo;
   struct wpa_sa sa;
   char tmp[512];

   /* analyze these only if we have a wpa key */
   if (GBL_WIFI->wifi_schema != WIFI_WPA)
      return NULL;

   /* get the EAPOL packet */
   DECODED_LEN = sizeof(struct eapol_header);
   eapol = (struct eapol_header *)DECODE_DATA;

   /* only interested in EAPOL KEY packets */
   if (eapol->type != EAPOL_KEY)
      return NULL;

   /* get the EAPOL KEY packet */
   eapol_key = (struct eapol_key_header *)(eapol + 1);
   DECODED_LEN += sizeof(struct eapol_key_header) + ntohs(eapol_key->key_data_len);

   /* only interested in RSN KEY (for CCMP) or WPA KEY (for TKIP) packets */
   if (eapol_key->type != EAPOL_KEY_RSN && eapol_key->type != EAPOL_KEY_WPA)
      return NULL;

   /* check the encryption algorithm */
   algo = eapol_enc_algo(eapol_key);

   /* check the 4-way handshake message number */
   message = eapol_4way_handshake(eapol_key);

   /* invalid packet */
   if (message <= 0) {
      return NULL;
   }

   /*
    * message 1 and 3 are from the AP
    * message 2 and 4 are from the STA
    * group keys come from AP
    */
   if (message == EAPOL_4WAY_MESSAGE_GROUP) {
      memcpy(sta, PACKET->L2.dst, ETH_ADDR_LEN);
      memcpy(bssid, PACKET->L2.src, ETH_ADDR_LEN);
   } if (message % 2) {
      memcpy(sta, PACKET->L2.dst, ETH_ADDR_LEN);
      memcpy(bssid, PACKET->L2.src, ETH_ADDR_LEN);
   } else {
      memcpy(sta, PACKET->L2.src, ETH_ADDR_LEN);
      memcpy(bssid, PACKET->L2.dst, ETH_ADDR_LEN);
   }

   mac_addr_ntoa(sta, sta_address);

   /* handle the Group Key message */
   if (message == EAPOL_4WAY_MESSAGE_GROUP) {

      USER_MSG("EAPOL: group key detected...\n");

      /*
       * the group key is encrypted and we need a valid completed sa to decrypt it
       * the sa must be at state four
       */
      if (wpa_sess_get(sta, &sa) == ESUCCESS && sa.state == EAPOL_4WAY_MESSAGE_FOUR) {
         rsn_ie = (struct rsn_ie_header *)(eapol_key + 1);
         wpa_decrypt_broadcast_key(eapol_key, rsn_ie, &sa);
      }

      return NULL;
   }


   USER_MSG("EAPOL packet: [%s] 4-way handshake %d [%s]\n", sta_address, message, (algo == WPA_KEY_TKIP) ? "TKIP (WPA)" : "CCMP (WPA2)");

   switch (message) {
      case EAPOL_4WAY_MESSAGE_ONE:
         /*
          * On reception of Message 1, the Supplicant determines whether the Key Replay Counter field value has been
          * used before with the current PMKSA. If the Key Replay Counter field value is less than or equal to the current
          * local value, the Supplicant discards the message.
          */

         /* save ANonce (from authenticator)  to derive the PTK with the SNonce (from the 2 message)   */
         memcpy(sa.ANonce, eapol_key->key_nonce, WPA_NONCE_LEN);

         DEBUG_MSG("WPA ANonce : %s", str_tohex(sa.ANonce, WPA_NONCE_LEN, tmp, sizeof(tmp)));

         /* get the Key Descriptor Version (to select algorithm used in decryption -CCMP or TKIP-)  */
         sa.algo = algo;

         /* remember the state of the 4-way handshake */
         sa.state = EAPOL_4WAY_MESSAGE_ONE;

         /* save the status of the SA, overwrite it if it was an old one */
         wpa_sess_add(sta, &sa);

         break;

      case EAPOL_4WAY_MESSAGE_TWO:
         /* retrieve the session for this STA and check that the sate is consistent */
         if (wpa_sess_get(sta, &sa) != ESUCCESS || sa.state != EAPOL_4WAY_MESSAGE_ONE)
            break;

         /*
          * On reception of Message 2, the Authenticator checks that the key replay counter corresponds to the
          * outstanding Message 1. If not, it silently discards the message.
          * If the calculated MIC does not match the MIC that the Supplicant included in the EAPOL-Key frame,
          * the Authenticator silently discards Message 2.
          */
         memcpy(sa.SNonce, eapol_key->key_nonce, WPA_NONCE_LEN);

         DEBUG_MSG("WPA SNonce : %s", str_tohex(sa.SNonce, WPA_NONCE_LEN, tmp, sizeof(tmp)));

         /* derive the PTK from the BSSID, STA MAC, PMK (WPA-PSK), SNonce, ANonce */
         wpa_generate_PTK(bssid, sta, GBL_WIFI->wkey, sa.SNonce, sa.ANonce, (sa.algo == WPA_KEY_TKIP) ? 512 : 384, sa.ptk);

         DEBUG_MSG("WPA PTK : %s", str_tohex(sa.ptk, WPA_PTK_LEN, tmp, sizeof(tmp)));

         /* verify the MIC (compare the MIC in the packet included in this message with a MIC calculated with the PTK) */
         if (wpa_check_MIC(eapol, eapol_key, DECODED_LEN, sa.ptk, sa.algo) != ESUCCESS) {
            USER_MSG("WPA MIC does not match\n");
            break;
         }

         DEBUG_MSG("WPA MIC : %s", str_tohex(eapol_key->key_MIC, WPA_MICKEY_LEN, tmp, sizeof(tmp)));

         /* remember the state of the 4-way handshake */
         sa.state = EAPOL_4WAY_MESSAGE_TWO;

         /* save the status of the SA, overwrite it if it was an old one */
         wpa_sess_add(sta, &sa);

         break;

      case EAPOL_4WAY_MESSAGE_THREE:
         /* retrieve the session for this STA and check that the sate is consistent */
         if (wpa_sess_get(sta, &sa) != ESUCCESS || sa.state != EAPOL_4WAY_MESSAGE_TWO)
            break;

         /*
          * On reception of Message 3, the Supplicant silently discards the message if the Key Replay Counter field
          * value has already been used or if the ANonce value in Message 3 differs from the ANonce value in Message 1
          * If using WPA2 PSK, message 3 will contain an RSN for the group key (GTK KDE).
          * In order to properly support decrypting WPA2-PSK packets, we need to parse this to get the group key.
          */
         rsn_ie = (struct rsn_ie_header *)(eapol_key + 1);

         wpa_decrypt_broadcast_key(eapol_key, rsn_ie, &sa);

         /* remember the state of the 4-way handshake */
         sa.state = EAPOL_4WAY_MESSAGE_THREE;

         /* save the status of the SA, overwrite it if it was an old one */
         wpa_sess_add(sta, &sa);

         break;

      case EAPOL_4WAY_MESSAGE_FOUR:
         /* retrieve the session for this STA and check that the sate is consistent */
         if (wpa_sess_get(sta, &sa) != ESUCCESS || sa.state != EAPOL_4WAY_MESSAGE_THREE)
            break;

         /*
          * On reception of Message 4, the Authenticator verifies that the Key Replay Counter field value is one
          * that it used on this 4-Way Handshake; if it is not, it silently discards the message.
          * If the calculated MIC does not match the MIC that the Supplicant included in the EAPOL-Key frame, the
          * Authenticator silently discards Message 4.
          */

         /* we are done ! just copy the decryption key from PTK */
         memcpy(sa.decryption_key, sa.ptk + 32, WPA_DEC_KEY_LEN);

         USER_MSG("WPA KEY : %s\n", str_tohex(sa.decryption_key, WPA_DEC_KEY_LEN, tmp, sizeof(tmp)));

         /* remember the state of the 4-way handshake */
         sa.state = EAPOL_4WAY_MESSAGE_FOUR;

         /* save the status of the SA, overwrite it if it was an old one */
         wpa_sess_add(sta, &sa);

         break;
   }


   return NULL;
}


static int eapol_4way_handshake(struct eapol_key_header *eapol_key)
{
   u_int16 key_info = ntohs(eapol_key->key_info);

   /* IEEE-802.11i-2004  ¤ 8.5.3 */

   /*
    * all the 4-way handshake packets have the pairwise bit set
    * if not, it is a Group Key, and should be handled in a different way
    */
   if ((key_info & WPA_KEY_PAIRWISE) == 0) {
      /* Group Key (Sec=0, Mic=0, Ack=1)  */
      if ((key_info & WPA_KEY_SECURE) == 0 &&
          (key_info & WPA_KEY_MIC) == 0 &&
          (key_info & WPA_KEY_ACK) != 0) {
         return EAPOL_4WAY_MESSAGE_GROUP;
      }
      return -ENOTHANDLED;
   }

   /* message 1: Authenticator->Supplicant (Sec=0, Mic=0, Ack=1, Inst=0, Key=1(pairwise), KeyRSC=0, Nonce=ANonce, MIC=0)  */
   if ((key_info & WPA_KEY_SECURE) == 0 &&
       (key_info & WPA_KEY_MIC) == 0 &&
       (key_info & WPA_KEY_ACK) != 0 &&
       (key_info & WPA_KEY_INSTALL) == 0)
      return EAPOL_4WAY_MESSAGE_ONE;

   /* message 2: Supplicant->Authenticator (Sec=0, Mic=1, Ack=0, Inst=0, Key=1(pairwise), KeyRSC=0, Nonce=SNonce, MIC=MIC(KCK,EAPOL))
    *    to distinguish between message 2 and 4, check the len of the key_data_len (should be != 0)
    */
   if ((key_info & WPA_KEY_SECURE) == 0 &&
       (key_info & WPA_KEY_MIC) != 0 &&
       (key_info & WPA_KEY_ACK) == 0 &&
       (key_info & WPA_KEY_INSTALL) == 0 &&
       eapol_key->key_data_len != 0)
      return EAPOL_4WAY_MESSAGE_TWO;

   /* message 3:
    *    CCMP: Authenticator->Supplicant (Sec=1/0, Mic=1, Ack=1, Inst=1, Key=1(pairwise), KeyRSC=???, Nonce=ANonce, MIC=1)
    *    TKIP: Authenticator->Supplicant (Sec=0, Mic=1, Ack=1, Inst=1, Key=1(pairwise), KeyRSC=???, Nonce=ANonce, MIC=1)
    */
   if ( /* ((key_info & WPA_KEY_TKIP) || (key_info & WPA_KEY_SECURE) != 0) && */
       (key_info & WPA_KEY_MIC) != 0 &&
       (key_info & WPA_KEY_ACK) != 0 &&
       (key_info & WPA_KEY_INSTALL) != 0)
      return EAPOL_4WAY_MESSAGE_THREE;

   /* message 4:
    *    CCMP: Supplicant->Authenticator (Sec=1/0, Mic=1, Ack=0, Inst=0, Key=1(pairwise), KeyRSC=0, Nonce=0, MIC=MIC(KCK,EAPOL))
    *    TKIP: Supplicant->Authenticator (Sec=0, Mic=1, Ack=0, Inst=0, Key=1(pairwise), KeyRSC=0, Nonce=0, MIC=MIC(KCK,EAPOL))
    *    to distinguish between message 2 and 4, check the len of the key_data_len (should be == 0)
    */
   if (/* ((key_info & WPA_KEY_TKIP) || (key_info & WPA_KEY_SECURE) != 0) && */
       (key_info & WPA_KEY_MIC) != 0 &&
       (key_info & WPA_KEY_ACK) == 0 &&
       (key_info & WPA_KEY_INSTALL) == 0 &&
       eapol_key->key_data_len == 0)
      return EAPOL_4WAY_MESSAGE_FOUR;

   /* invalid packet */
   return -ENOTHANDLED;
}

static int eapol_enc_algo(struct eapol_key_header *eapol_key)
{
   u_int16 key_info = ntohs(eapol_key->key_info);

   if ((key_info & WPA_KEY_TKIP) != 0)
      return WPA_KEY_TKIP;

   if ((key_info & WPA_KEY_CCMP) != 0)
      return WPA_KEY_CCMP;

   return 0;
}

/* EOF */

// vim:ts=3:expandtab

