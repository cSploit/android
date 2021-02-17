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

*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>
#include <ec_capture.h>
#include <ec_checksum.h>
#include <ec_encryption.h>

#ifdef HAVE_OPENSSL
   #include <openssl/rc4.h>
   #include <openssl/md5.h>
#endif


/* globals */

struct __attribute__ ((__packed__)) wifi_header {
   u_int8   type;
      #define WIFI_DATA    0x08
      #define WIFI_BACON   0x80
      #define WIFI_ACK     0xd4
   u_int8   control;
      #define WIFI_STA_TO_STA 0x00  /* ad hoc mode */
      #define WIFI_STA_TO_AP  0x01
      #define WIFI_AP_TO_STA  0x02
      #define WIFI_AP_TO_AP   0x03
      #define WIFI_ENCRYPTED  0x40
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
   /* u_int8   ha4[ETH_ADDR_LEN]; */
   /* this field is present only when WIFI_DATA and WIFI_BACON are set (802.11 QoS Data) */
   /* u_int6 qos; */
};

struct __attribute__ ((__packed__)) llc_header {
   u_int8   dsap;
   u_int8   ssap;
   u_int8   control;
   u_int8   org_code[3];
   u_int16  proto;
};



/* encapsulated ethernet */
static u_int8 WIFI_ORG_CODE[3] = {0x00, 0x00, 0x00};


/* protos */

FUNC_DECODER(decode_wifi);
FUNC_ALIGNER(align_wifi);
void wifi_init(void);


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
   struct wifi_header *wifi = NULL;
   struct llc_header *llc = NULL;
   struct wep_header *wep = NULL;
   struct wpa_header *wpa = NULL;
   FUNC_DECODER_PTR(next_decoder) = NULL;
   u_char *wifi_end = NULL;
   u_char enc_schema;

   DECODED_LEN = sizeof(struct wifi_header);

   /* if the packet has an FCS at the end, subtract it to the size of the data to be analyzed */
   if ((PACKET->L2.flags & PO_L2_FCS)) {
      DECODE_DATALEN -= 4;
   }

   /* get the ieee802.11 header */
   wifi = (struct wifi_header *)DECODE_DATA;
   wifi_end = (u_char *)wifi + sizeof(struct wifi_header);

   /* we are interested only in wifi data packets */
   if (!(wifi->type & WIFI_DATA)) {
      DEBUG_MSG("WIFI bacon");
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
         /* there is one more field (ha4) in this case */
         memcpy(PACKET->L2.src, (char *)(wifi + 1), ETH_ADDR_LEN);
         memcpy(PACKET->L2.dst, wifi->ha3, ETH_ADDR_LEN);

         if (wifi->control & WIFI_ENCRYPTED) {
            // XXX - implement AP to AP handling in encryption_ccmp
            DEBUG_MSG("WIFI: encrypted packets (AP to AP) not supported");
            return NULL;
         }
         /* increase the end of the header accordingly */
         wifi_end += ETH_ADDR_LEN;
         DECODED_LEN += ETH_ADDR_LEN;
         break;

      default:
         return NULL;
   }

   /*
    * if the packet is marked as a BACON, it means that it is a DATA + BACON
    * used in (802.11 QoS Data)
    * we have to skip the 2 byte QoS field in the header
    */
   if (wifi->type & WIFI_BACON) {
      wifi_end += sizeof(u_int16);
      DECODED_LEN += sizeof(u_int16);
   }

   /*
    * check the type of encryption
    * the third byte of the IV contains the info
    */
   if (((*(wifi_end + 3) >> 5) & 0x01) == 0x01) {
      enc_schema = WIFI_WPA;
   } else {
      enc_schema = WIFI_WEP;
   }

   /*
    * the frame is encrypted.
    * check if the provided key is compatible with the schema
    */
   if (wifi->control & WIFI_ENCRYPTED && enc_schema == WIFI_WEP && GBL_WIFI->wifi_schema == WIFI_WEP) {

      DEBUG_MSG("%s: encrypted packet wep", __FUNCTION__);

      /* get the WEP header */
      wep = (struct wep_header *)wifi_end;
      DECODED_LEN += sizeof(struct wep_header);

      if (DECODED_LEN >= DECODE_DATALEN) {
         return NULL;
      }

      if ((DECODE_DATALEN - DECODED_LEN) > UINT16_MAX) {
         return NULL;
      }

      /* decrypt the packet */
      if (wep_decrypt((u_char *)wep, DECODE_DATALEN - DECODED_LEN, GBL_WIFI->wkey, GBL_WIFI->wkey_len) != ESUCCESS)
         return NULL;

      /* the wep header was overwritten, remove it from the decoded portion */
      DECODED_LEN -= sizeof(struct wep_header);

      /* remove the encrypted bit from the header since the data are now decrypted */
      wifi->control &= ~WIFI_ENCRYPTED;
   }

   if (wifi->control & WIFI_ENCRYPTED && enc_schema == WIFI_WPA && GBL_WIFI->wifi_schema == WIFI_WPA) {
      struct wpa_sa sa;

      DEBUG_MSG("%s: encrypted packet wpa", __FUNCTION__);

      /*
       * get the SA for this STA (search for source or dest mac address)
       * if we don't have a valid SA there is no way to decrypt the packet.
       * the SA is calculated from the 4-way handshake into the EAPOL packets
       */
      if (wpa_sess_get(PACKET->L2.src, &sa) == ESUCCESS || wpa_sess_get(PACKET->L2.dst, &sa) == ESUCCESS) {

         /* get the WPA header (CCMP or TKIP initialization vector) */
         wpa = (struct wpa_header *)wifi_end;
         DECODED_LEN += sizeof(struct wpa_header);

         if (DECODED_LEN >= DECODE_DATALEN) {
            return NULL;
         }

         if ((DECODE_DATALEN - DECODED_LEN) > UINT16_MAX) {
             return NULL;
         }

         /* decrypt the packet */
         if (wpa_decrypt((u_char *)wifi, (u_char *)wpa, DECODE_DATALEN - DECODED_LEN, sa) != ESUCCESS) {
            return NULL;
         }

         /* the wpa header was overwritten, remove it from the decoded portion */
         DECODED_LEN -= sizeof(struct wpa_header);

         /* remove the encryption bit from the header since the data are now decrypted */
         wifi->control &= ~WIFI_ENCRYPTED;
      }
   }

   /* if the packet is still encrypted, skip it */
   if (wifi->control & WIFI_ENCRYPTED) {
      return NULL;
   }

   /* get the logical link layer header */
   llc = (struct llc_header *)wifi_end;
   DECODED_LEN += sizeof(struct llc_header);

   /* "Organization Code" different from "Encapsulated Ethernet" not yet supported */
   if (memcmp(llc->org_code, WIFI_ORG_CODE, 3)) {
      return NULL;
   }

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

/* EOF */

// vim:ts=3:expandtab

