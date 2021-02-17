

#ifndef EC_ENCRYPTION_H
#define EC_ENCRYPTION_H

#include <ec_inet.h>

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

#define WEP_IV_LEN 3
#define WEP_CRC_LEN 4

#define WPA_IV_LEN 8
#define MAX_WKEY_LEN   32 /* 256 bit */

struct wep_header {
   u_int8 init_vector[WEP_IV_LEN];
   u_int8 key;
};

struct wpa_header {
   u_int8 init_vector[WPA_IV_LEN];
};

/*
 * http://etutorials.org/Networking/802.11+security.+wi-fi+protected+access+and+802.11i/Part+II+The+Design+of+Wi-Fi+Security/Chapter+10.+WPA+and+RSN+Key+Hierarchy/Details+of+Key+Derivation+for+WPA/
 *
 * http://en.wikipedia.org/wiki/IEEE_802.11i-2004
 *
 */

#define WPA_KEY_LEN     32
#define WPA_NONCE_LEN   32
#define WPA_PTK_LEN     64 /* TKIP uses 48 bytes, CCMP uses 64 bytes */
#define WPA_MICKEY_LEN  16
#define WPA_KCK_LEN     16
#define WPA_DEC_KEY_LEN 16

#define WPA_CCMP_TRAILER 8

struct __attribute__ ((__packed__)) rsn_ie_header {
   u_int8  id;
   u_int8  len;
   u_int8  OUI[4];
   u_int16 version;
   u_int8  multicastOUI[4];
   u_int16 iUnicastCount;  /* this should always be 1 for WPA client */
   u_int8  unicastOUI[4];
   u_int16 iAuthCount;     /* this should always be 1 for WPA client */
   u_int8  authOUI[4];
   u_int16 wpa_cap;
};

struct __attribute__ ((__packed__)) eapol_header {
   u_int8  version;
   u_int8  type;
      #define EAP_PACKET   0x00
      #define EAPOL_START  0x01
      #define EAPOL_LOGOFF 0x02
      #define EAPOL_KEY    0x03
      #define EAPOL_ENCAP  0x04
   u_int16 len;
};

struct __attribute__ ((__packed__)) eapol_key_header {
   u_int8  type;
      #define EAPOL_KEY_RSN 0x02
      #define EAPOL_KEY_WPA 0xfe
   u_int16 key_info;
      #define WPA_KEY_TKIP     0x0001
      #define WPA_KEY_CCMP     0x0002
      #define WPA_KEY_PAIRWISE 0x0008
      #define WPA_KEY_INSTALL  0x0040
      #define WPA_KEY_ACK      0x0080
      #define WPA_KEY_MIC      0x0100
      #define WPA_KEY_SECURE   0x0200
      #define WPA_KEY_ENCRYPT  0x1000
   u_int16 key_len;
   u_int8  replay_counter[8];
   u_int8  key_nonce[32];
   u_int8  key_IV[16];
   u_int8  key_RSC[8];
   u_int8  key_ID[8];
   u_int8  key_MIC[16];
   u_int16 key_data_len;
};


struct wpa_sa {
   struct timeval tv;           /* used to timeout the session */
   u_char state;
   u_char algo;
   u_char SNonce[WPA_NONCE_LEN];
   u_char ANonce[WPA_NONCE_LEN];
   u_char ptk[WPA_PTK_LEN * 2]; /* 512 bit is the max PRF will output, multiply the space for key calculation */
   u_char decryption_key[WPA_DEC_KEY_LEN];
};

struct wpa_session {
   u_char sta[ETH_ADDR_LEN];        /* the STA mac address that is associating */
   struct wpa_sa sa;
   LIST_ENTRY (wpa_session) next;
};


#define XOR_BLOCK(b, a, len)        \
{                                   \
   int i;                           \
   for (i = 0; i < (int)(len); i++) \
      (b)[i] ^= (a)[i];             \
}

/* forward declaration in wifi_eapol.c */
struct rsn_ie_header;
struct eapol_header;
struct eapol_key_header;

extern int wep_decrypt(u_char *buf, size_t len, u_char *wkey, size_t wlen);
extern int wifi_key_prepare(char *key_string);

extern void wpa_sess_add(u_char *sta, struct wpa_sa *sa);
extern void wpa_sess_del(u_char *sta);
extern int wpa_sess_get(u_char *sta, struct wpa_sa *sa);

extern int wpa_generate_PTK(u_char *bssid, u_char *sta, u_char *pmk, u_char *snonce, u_char *anonce, u_int16 bits, u_char *ptk);
extern int wpa_check_MIC(struct eapol_header *eapol, struct eapol_key_header* eapol_key, size_t eapol_len, u_char *kck, int algo);
extern int wpa_decrypt_broadcast_key(struct eapol_key_header *eapol_key, struct rsn_ie_header *rsn_ie, struct wpa_sa *sa);
extern int wpa_decrypt(u_char *mac, u_char *data, size_t len, struct wpa_sa sa);

#endif

/* EOF */

// vim:ts=3:expandtab
