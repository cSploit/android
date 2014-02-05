/*
    ettercap -- dissector gadu-gadu -- TCP 8074 (alternatively 443)

    Copyright (C) Michal Szymanski <michal.szymanski.pl@gmail.com>
                                   gg: 7244283

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

/*

ABOUT:

ettercap-gg is a Gadu-Gadu IM ettercap dissector. 

It is a patch for ettercap sniffer that adds the ability to intercept Gadu-Gadu logins, passwords and messages.

Gadu-Gadu (http://www.gadu-gadu.pl/) is the most widely used IM network in Poland with ~6mln users.

Protocol description taken from http://ekg.chmurka.net/docs/protocol.html + own research (7.x).

The newest version can be found at http://ettercap-gg.sourceforge.net/

FEATURES:

- supports following gadu-gadu protocols: 4.x, 5.x, 6.x, 7.x
- intercepts sent/received messages
- intercepts gg numbers, password hashes and seeds (can be bruteforced by ggbrute)
- intercepts status descriptions
- notifies about status changes
- intercepts gg server/client ip addresses
- intercepts gg user's local/remote ip addresses
- intercepts gg connections to port 8074 and 443
- determines Gadu-Gadu version

INSTALLATION & CONFIGURATION: 

Apply Gadu-Gadu dissector patch and compile ettercap as you used to do before:

patch -p0 < ettercap-NG-0.7.3-gg_dissector_02.patch
./configure
make
make install

Normally Gadu-Gadu dissector is installed on port number 8074 (appropriate entry is added to etter.conf file). If you want to enable dissector to 
intercept traffic on port 443 just turn off https dissector (there can be only one dissector on the same port at the same time) by editing etter.conf 
file and changing following line:

https = 443              # tcp    443

to

https = 0                # tcp    443

After that all you need to do is to add 443 port to gg dissector:

gg = 8074,443            # tcp    8074

That's all. Play wisely.

*/

/*

EXAMPLE SESSION (with GG_CONTACTS_STATUS_CHANGES enabled) - version 0.2:

ARP poisoning victims:

 GROUP 1 : 10.10.10.11 00:01:20:02:34:21

 GROUP 2 : 10.10.10.1 00:0A:84:D8:28:F5

Starting Unified sniffing...

Text only Interface activated...
Hit 'h' for inline help

GG : 217.17.45.143:443 -> 10.10.10.11:1696 - WELCOME  SEED: 0xAD130562 (2903704930)
GG7 : 10.10.10.11:1696 -> 217.17.45.143:443 - LOGIN  UIN: 5114529  PWD_HASH: 0x21D13E38992A341DD33BB52DDFA2382A173A5361  STATUS:  (invisible + private)  VERSION: 7.7  LIP: 10.10.10.11:1550  RIP: 0.0.0.0:0
GG : 10.10.10.11:1706 -> 217.17.45.143:443 - SEND_MSG  RECIPIENT: 7244283  MESSAGE: "wiadomosc testowa"
GG : 217.17.45.143:443 -> 10.10.10.11:1706 - RECV_MSG  SENDER: 7244283  MESSAGE: "dzieki za wiadomosc"
GG7 : 217.17.45.143:443 -> 10.10.10.11:1706 - STATUS CHANGED  UIN: 7244283  STATUS: a swistak siedzi i zawija (busy + descr)  VERSION: 7.6  RIP: 207.46.19.190:1550

GG : 217.17.41.85:8074 -> 10.10.10.11:1685 - WELCOME  SEED: 0x1D66B45F (493270111)
GG4/5 : 10.10.10.11:1685 -> 217.17.41.85:8074 - LOGIN  UIN: 5114529  PWD_HASH: 0x1B85493D (461719869)  STATUS: zaraz weekend (invisible + descr + private)  VERSION: 4.8 + has audio  LIP: 10.10.10.11:1550
GG : 217.17.41.85:8074 -> 10.10.10.11:1685 - STATUS CHANGED  UIN: 2688291  STATUS: i co ja bede robil przez te 4 dni ... (not available + descr)
GG : 10.10.10.11:1685 -> 217.17.41.85:8074 - NEW STATUS  STATUS: goraaaaaaaaaaaaaco!!!!!!!!! (busy + descr + private)

*/

/*

CHANGELOG:

v0.2:

- added interception of sent/received messages
- added interception of status descriptions
- added notification about status changes
- added interception of gg server/client ip addresses
- added interception of gg user's local/remote ip addresses
- added determination of Gadu-Gadu version
- tiny bugfixes

v0.1 (initial release):

- added support for following gadu-gadu protocols: 4.x, 5.x, 6.x, 7.x 
- added interception of gg numbers, password hashes and seeds
- added interception of gg connections to port 8074 and 443 

*/

/*

TODO:

- wpkontakt support (sessions management needed)
- std_gg/kadu/ekg/wpkontakt fingerprinting (additional research needed)
- sms sniffing? (already implemented through http dissector)
- nat detection

*/

/* CONFIGURATION */

/* 
uncomment #define below if you want to see all contacts status changes - be careful using this option in really big networks - it could mess up your whole screen !
*/
/*
#define GG_CONTACTS_STATUS_CHANGES  1
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>

/* globals */

#define GG_LOGIN50_CMD		0x0000000c
#define GG_LOGIN60_CMD		0x00000015
#define GG_LOGIN70_CMD		0x00000019

#define GG_WELCOME_CMD		0x00000001

#define GG_SEND_MSG_CMD		0x0000000b
#define GG_RECV_MSG_CMD		0x0000000a

#define GG_NEW_STATUS_CMD  0x00000002

#define GG_STATUS_CMD      0x00000002

#define GG_STATUS50_CMD		0x0000000c
#define GG_STATUS60_CMD    0x0000000f
#define GG_STATUS70_CMD    0x00000017

struct gg_hdr {
   u_int32 type;
   u_int32 len;
};

struct gg_welcome_hdr {
   u_int32 seed;
};

struct gg_login50_hdr {
   u_int32 uin;       
   u_int32 hash;      
   u_int32 status;    
   u_int32 version;   
   u_int8 local_ip[4];  
   u_int16 local_port;
   char description[71];     /* 70+1 (0x0) */
   u_int32 time;
}__attribute__ ((packed));

struct gg_login60_hdr {
   u_int32 uin;       
   u_int32 hash;      
   u_int32 status;    
   u_int32 version;   
   u_int8 unknown1;          /* 0x00 */
   u_int8 local_ip[4];  
   u_int16 local_port;
   u_int8 remote_ip[4]; 
   u_int16 remote_port;
   u_int8 image_size;
   u_int8 unknown2;          /* 0xbe */
   char description[71];     /* 70+1 (0x0) */
   u_int32 time;
}__attribute__ ((packed));

struct gg_login70_hdr {
   u_int32 uin;       
   u_int8 unknown1;          /* 0x02 */
   u_int32 hash[5];          /* 160b */      
   u_int32 unknown2[11];     /* 0x00, 352b */
   u_int32 status;    
   u_int16 version;   
   u_int16 unknown3;         /* 0xc202 */
   u_int8 unknown4;          /* 0x00 */
   u_int8 local_ip[4];  
   u_int16 local_port;
   u_int8 remote_ip[4]; 
   u_int16 remote_port;
   u_int8 image_size;
   u_int8 unknown5;          /* 0xbe */
   char description[71];     /* 70+1 (0x0) */
   u_int32 time;
}__attribute__ ((packed));

struct gg_send_msg_hdr {
   u_int32 recipient;
   u_int32 seq;
   u_int32 class;
   char message[2000];
}__attribute__ ((packed));

struct gg_recv_msg_hdr {
   u_int32 sender;
   u_int32 seq;
   u_int32 time;
   u_int32 class;
   char message[2000];
}__attribute__ ((packed));

struct gg_new_status_hdr {
   u_int32 status;
   char description[71];     /* 70+1 (0x0) */
   u_int32 time;
};

struct gg_status_hdr {
   u_int32 uin;
   u_int32 status;
   char description[71];     /* 70+1 (0x0) */
   u_int32 time;
};

struct gg_status50_hdr {
   u_int32 uin;       
   u_int32 status;    
   u_int8 remote_ip[4]; 
   u_int16 remote_port;
   u_int32 version;   
   u_int16 unknown1;         /* remote port again? */
   char description[71];     /* 70+1 (0x0) */
   u_int32 time;
}__attribute__ ((packed));

struct gg_status60_hdr {
   u_int32 uin;
   u_int8 status;
   u_int8 remote_ip[4]; 
   u_int16 remote_port;
   u_int8 version;   
   u_int8 image_size;
   u_int8 unknown1;          /* 0x00 */
   char description[71];     /* 70+1 (0x0) */
   u_int32 time;
}__attribute__ ((packed));;

struct gg_status70_hdr {
   u_int32 uin;
   u_int8 status;
   u_int8 remote_ip[4]; 
   u_int16 remote_port;
   u_int8 version;   
   u_int8 image_size;
   u_int8 unknown1;          /* 0x00 */
   u_int32 unknown2;         /* 0x00 */
   char description[71];     /* 70+1 (0x0) */
   u_int32 time;
}__attribute__ ((packed));;

/* protos */

FUNC_DECODER(dissector_gg);
void gg_init(void);
void gg_get_status(u_int32 status, char *str);
void gg_get_version(u_int32 version, char *str);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init gg_init(void)
{
   dissect_add("gg", APP_LAYER_TCP, 8074, dissector_gg);
/*   dissect_add("gg", APP_LAYER_TCP, 443, dissector_gg); */
}

FUNC_DECODER(dissector_gg)
{
   DECLARE_DISP_PTR_END(ptr, end);
   char tmp[MAX_ASCII_ADDR_LEN];
   char tmp2[MAX_ASCII_ADDR_LEN];
   struct gg_hdr *gg;
   struct gg_welcome_hdr *gg_welcome;
   struct gg_login50_hdr *gg_login50;
   struct gg_login60_hdr *gg_login60;
   struct gg_login70_hdr *gg_login70;
   struct gg_send_msg_hdr *gg_send_msg;
   struct gg_recv_msg_hdr *gg_recv_msg;
   struct gg_new_status_hdr *gg_new_status;
   char *tbuf, *tbuf2, *tbuf3;
   char user[10], pass[40];

   /* don't complain about unused var */
   (void)end;
   
   /* skip empty packets (ACK packets) */
   if (PACKET->DATA.len == 0)
      return NULL;
   
   /* cast the gg header */
   gg = (struct gg_hdr *)ptr;
   gg_login50 = (struct gg_login50_hdr *)(gg + 1);
   gg_login60 = (struct gg_login60_hdr *)(gg + 1);
   gg_login70 = (struct gg_login70_hdr *)(gg + 1);
   gg_welcome = (struct gg_welcome_hdr *)(gg + 1);
   gg_send_msg = (struct gg_send_msg_hdr *)(gg + 1);
   gg_recv_msg = (struct gg_recv_msg_hdr *)(gg + 1);
   gg_new_status = (struct gg_new_status_hdr *)(gg + 1);

   /* what type of packets do we process ? */
   if (gg->type != GG_LOGIN50_CMD && gg->type != GG_LOGIN60_CMD && gg->type != GG_LOGIN70_CMD && gg->type != GG_WELCOME_CMD && gg->type != GG_SEND_MSG_CMD && gg->type != GG_RECV_MSG_CMD && gg->type != GG_NEW_STATUS_CMD && gg->type != GG_STATUS_CMD && gg->type != GG_STATUS50_CMD && gg->type != GG_STATUS60_CMD && gg->type != GG_STATUS70_CMD)
      return NULL;

   /* skip incorrect Gadu-Gadu packets */
   if ((gg->len) != ((PACKET->DATA.len)-8))
      return NULL;

   DEBUG_MSG("Gadu-Gadu --> TCP dissector_gg (%.8X:%.8X)", gg->type, gg->len);

   SAFE_CALLOC(tbuf, 50, sizeof(char));
   SAFE_CALLOC(tbuf2, 71, sizeof(char));
   SAFE_CALLOC(tbuf3, 30, sizeof(char));
   
if ((gg->type == GG_LOGIN50_CMD) && !FROM_SERVER("gg", PACKET)) {
   gg_get_status(gg_login50->status,tbuf);
   gg_get_version(gg_login50->version,tbuf3);
   strncpy(tbuf2,gg_login50->description, (gg->len)-22);
   tbuf2[(gg->len)-22]='\0';
   sprintf(user,"%u",gg_login50->uin);
   PACKET->DISSECTOR.user = strdup(user);
   sprintf(pass,"%u",gg_login50->hash);
   PACKET->DISSECTOR.pass = strdup(pass);
   DISSECT_MSG("GG4/5 : %s:%d -> %s:%d - LOGIN  UIN: %u  PWD_HASH: 0x%.8X (%u)  STATUS: %s (%s)  VERSION: %s  LIP: %u.%u.%u.%u:%u\n", ip_addr_ntoa(&PACKET->L3.src, tmp),
                                 ntohs(PACKET->L4.src), 
                                 ip_addr_ntoa(&PACKET->L3.dst, tmp2),
                                 ntohs(PACKET->L4.dst),
                                 gg_login50->uin,
                                 gg_login50->hash, gg_login50->hash, 
                                 tbuf2, tbuf,
                                 tbuf3,
                                 gg_login50->local_ip[0], gg_login50->local_ip[1], gg_login50->local_ip[2], gg_login50->local_ip[3],
                                 gg_login50->local_port);
}
else if (gg->type == GG_LOGIN60_CMD) {
   gg_get_status(gg_login60->status,tbuf);
   gg_get_version(gg_login60->version,tbuf3);
   strncpy(tbuf2,gg_login60->description, (gg->len)-31);
   tbuf2[(gg->len)-31]='\0';
   sprintf(user,"%u",gg_login60->uin);
   PACKET->DISSECTOR.user = strdup(user);
   sprintf(pass,"%u",gg_login60->hash);
   PACKET->DISSECTOR.pass = strdup(pass);
   DISSECT_MSG("GG6 : %s:%d -> %s:%d - LOGIN  UIN: %u  PWD_HASH: 0x%.8X (%u)  STATUS: %s (%s)  VERSION: %s  LIP: %u.%u.%u.%u:%u  RIP: %u.%u.%u.%u:%u\n", ip_addr_ntoa(&PACKET->L3.src, tmp),
                                 ntohs(PACKET->L4.src), 
                                 ip_addr_ntoa(&PACKET->L3.dst, tmp2),
                                 ntohs(PACKET->L4.dst),
                                 gg_login60->uin,
                                 gg_login60->hash, gg_login60->hash, 
                                 tbuf2, tbuf,
                                 tbuf3,
                                 gg_login60->local_ip[0], gg_login60->local_ip[1], gg_login60->local_ip[2], gg_login60->local_ip[3],
                                 gg_login60->local_port,
                                 gg_login60->remote_ip[0], gg_login60->remote_ip[1], gg_login60->remote_ip[2], gg_login60->remote_ip[3],
                                 gg_login60->remote_port);
}
else if (gg->type == GG_LOGIN70_CMD) {
   gg_get_status(gg_login70->status,tbuf);
   gg_get_version(gg_login70->version,tbuf3);
   strncpy(tbuf2,gg_login70->description, (gg->len)-92);
   tbuf2[(gg->len)-92]='\0';
   sprintf(user,"%u",gg_login70->uin);
   PACKET->DISSECTOR.user = strdup(user);
   sprintf(pass,"%X%X%X%X%X",gg_login70->hash[0],gg_login70->hash[1],gg_login70->hash[2],gg_login70->hash[3],gg_login70->hash[4]);
   PACKET->DISSECTOR.pass = strdup(pass);
   DISSECT_MSG("GG7 : %s:%d -> %s:%d - LOGIN  UIN: %u  PWD_HASH: 0x%.8X%.8X%.8X%.8X%.8X  STATUS: %s (%s)  VERSION: %s  LIP: %u.%u.%u.%u:%u  RIP: %u.%u.%u.%u:%u\n", ip_addr_ntoa(&PACKET->L3.src, tmp),
                                 ntohs(PACKET->L4.src), 
                                 ip_addr_ntoa(&PACKET->L3.dst, tmp2),
                                 ntohs(PACKET->L4.dst),
                                 gg_login70->uin,
                                 gg_login70->hash[4],gg_login70->hash[3],gg_login70->hash[2],gg_login70->hash[1],gg_login70->hash[0],
                                 tbuf2, tbuf,
                                 tbuf3,
                                 gg_login70->local_ip[0], gg_login70->local_ip[1], gg_login70->local_ip[2], gg_login70->local_ip[3],
                                 gg_login70->local_port,
                                 gg_login70->remote_ip[0], gg_login70->remote_ip[1], gg_login70->remote_ip[2], gg_login70->remote_ip[3],
                                 gg_login70->remote_port);
}
else if (gg->type == GG_SEND_MSG_CMD) {
   if (!FROM_SERVER("gg", PACKET)) {
      DISSECT_MSG("GG : %s:%d -> %s:%d - SEND_MSG  RECIPIENT: %u  MESSAGE: \"%s\"\n", ip_addr_ntoa(&PACKET->L3.src, tmp),
                                 ntohs(PACKET->L4.src), 
                                 ip_addr_ntoa(&PACKET->L3.dst, tmp2),
                                 ntohs(PACKET->L4.dst),
                                 gg_send_msg->recipient,
                                 gg_send_msg->message);
   }
}
else if (gg->type == GG_RECV_MSG_CMD) {
   DISSECT_MSG("GG : %s:%d -> %s:%d - RECV_MSG  SENDER: %u  MESSAGE: \"%s\"\n", ip_addr_ntoa(&PACKET->L3.src, tmp),
                                 ntohs(PACKET->L4.src), 
                                 ip_addr_ntoa(&PACKET->L3.dst, tmp2),
                                 ntohs(PACKET->L4.dst),
                                 gg_recv_msg->sender,
                                 gg_recv_msg->message);
}
else if (gg->type == GG_WELCOME_CMD) {
   DISSECT_MSG("GG : %s:%d -> %s:%d - WELCOME  SEED: 0x%.8X (%u)\n", ip_addr_ntoa(&PACKET->L3.src, tmp),
                                 ntohs(PACKET->L4.src),
                                 ip_addr_ntoa(&PACKET->L3.dst, tmp2),
                                 ntohs(PACKET->L4.dst), 
                                 gg_welcome->seed, gg_welcome->seed);
}
#ifdef GG_CONTACTS_STATUS_CHANGES
else if ((gg->type == GG_STATUS_CMD) && FROM_SERVER("gg", PACKET)) {
    gg_get_status(gg_status->status,tbuf);
    strncpy(tbuf2,gg_status->description, (gg->len)-8);
    tbuf2[(gg->len)-8]='\0';
    DISSECT_MSG("GG : %s:%d -> %s:%d - STATUS CHANGED  UIN: %u  STATUS: %s (%s)\n", ip_addr_ntoa(&PACKET->L3.src, tmp),
                                 ntohs(PACKET->L4.src),
                                 ip_addr_ntoa(&PACKET->L3.dst, tmp2),
                                 ntohs(PACKET->L4.dst), 
                                 gg_status->uin,
                                 tbuf2, tbuf);
} 
#endif
else if ((gg->type == GG_NEW_STATUS_CMD) && !FROM_SERVER("gg", PACKET)) {
      gg_get_status(gg_new_status->status,tbuf);
      strncpy(tbuf2,gg_new_status->description, (gg->len)-4);
      tbuf2[(gg->len)-4]='\0';
      DISSECT_MSG("GG : %s:%d -> %s:%d - NEW STATUS  STATUS: %s (%s)\n", ip_addr_ntoa(&PACKET->L3.src, tmp),
                                 ntohs(PACKET->L4.src),
                                 ip_addr_ntoa(&PACKET->L3.dst, tmp2),
                                 ntohs(PACKET->L4.dst), 
                                 tbuf2, tbuf);
}
#ifdef GG_CONTACTS_STATUS_CHANGES
else if ((gg->type == GG_STATUS50_CMD) && FROM_SERVER("gg", PACKET)) {
      gg_get_status(gg_status50->status,tbuf);
      gg_get_version(gg_status50->version,tbuf3);
      strncpy(tbuf2,gg_status50->description, (gg->len)-20);
      tbuf2[(gg->len)-20]='\0';
      DISSECT_MSG("GG4/5 : %s:%d -> %s:%d - STATUS CHANGED  UIN: %u  STATUS: %s (%s)  VERSION: %s  RIP: %u.%u.%u.%u:%u\n", ip_addr_ntoa(&PACKET->L3.src, tmp),
                                 ntohs(PACKET->L4.src),
                                 ip_addr_ntoa(&PACKET->L3.dst, tmp2),
                                 ntohs(PACKET->L4.dst), 
                                 gg_status50->uin,
                                 tbuf2, tbuf,
                                 tbuf3,
                                 gg_status50->remote_ip[0], gg_status50->remote_ip[1], gg_status50->remote_ip[2], gg_status50->remote_ip[3],
                                 gg_status50->remote_port);
}
else if (gg->type == GG_STATUS60_CMD) {
      gg_get_status(gg_status60->status,tbuf);
      gg_get_version(gg_status60->version,tbuf3);
      strncpy(tbuf2,gg_status60->description, (gg->len)-14);
      tbuf2[(gg->len)-14]='\0';
      DISSECT_MSG("GG6 : %s:%d -> %s:%d - STATUS CHANGED  UIN: %u  STATUS: %s (%s)  VERSION: %s  RIP: %u.%u.%u.%u:%u\n", ip_addr_ntoa(&PACKET->L3.src, tmp),
                                 ntohs(PACKET->L4.src),
                                 ip_addr_ntoa(&PACKET->L3.dst, tmp2),
                                 ntohs(PACKET->L4.dst), 
                                 (gg_status60->uin & 0x00ffffff),
                                 tbuf2, tbuf,
                                 tbuf3,
                                 gg_status60->remote_ip[0], gg_status60->remote_ip[1], gg_status60->remote_ip[2], gg_status60->remote_ip[3],
                                 gg_status60->remote_port);
}
else if (gg->type == GG_STATUS70_CMD) {
      gg_get_status(gg_status70->status,tbuf);
      gg_get_version(gg_status70->version,tbuf3);
      strncpy(tbuf2,gg_status70->description, (gg->len)-18);
      tbuf2[(gg->len)-18]='\0';
      DISSECT_MSG("GG7 : %s:%d -> %s:%d - STATUS CHANGED  UIN: %u  STATUS: %s (%s)  VERSION: %s  RIP: %u.%u.%u.%u:%u\n", ip_addr_ntoa(&PACKET->L3.src, tmp),
                                 ntohs(PACKET->L4.src),
                                 ip_addr_ntoa(&PACKET->L3.dst, tmp2),
                                 ntohs(PACKET->L4.dst), 
                                 (gg_status70->uin & 0x00ffffff),
                                 tbuf2, tbuf,
                                 tbuf3,
                                 gg_status70->remote_ip[0], gg_status70->remote_ip[1], gg_status70->remote_ip[2], gg_status70->remote_ip[3],
                                 gg_status70->remote_port);
}
#endif
/*
else {
   DISSECT_MSG("GG : %s:%d -> %s:%d - CMD TYPE: %.8X (%d)  LEN: %.8X (%d)  DLEN: %d\n", ip_addr_ntoa(&PACKET->L3.src, tmp),
                                 ntohs(PACKET->L4.src),
                                 ip_addr_ntoa(&PACKET->L3.dst, tmp2),
                                 ntohs(PACKET->L4.dst),
                                 gg->type, gg->type, gg->len, gg->len, PACKET->DATA.len);
}
*/

   SAFE_FREE(tbuf);
   SAFE_FREE(tbuf2);
   SAFE_FREE(tbuf3);
  
   return NULL;
}

void gg_get_status(u_int32 status, char *str)  {

switch ((status&0x00ff)) {

case 0x0014:
   strcpy(str,"invisible");
   break;

case 0x0002:
   strcpy(str,"available");
   break;

case 0x0003:
   strcpy(str,"busy");
   break;

case 0x0006:
   strcpy(str,"blocked");
   break;

case 0x0001:
   strcpy(str,"not available");
   break;

case 0x0016:
   strcpy(str,"invisible + descr");
   break;

case 0x0004:
   strcpy(str,"available + descr");
   break;

case 0x0005:
   strcpy(str,"busy + descr");
   break;

case 0x0015:
   strcpy(str,"not available + descr");
   break;

default:
   strcpy(str,"unknown");
}

if ((status&0xff00)==0x8000)
   strcat(str," + private");

}

void gg_get_version(u_int32 version, char *str)  {

switch ((version&0x00ff)) {

case 0x0000002A:
   strcpy(str,"7.7");
   break;

case 0x00000029:
   strcpy(str,"7.6");
   break;

case 0x00000024:
   strcpy(str,"6.1/7.6");
   break;

case 0x00000028:
   strcpy(str,"7.5");
   break;

case 0x00000027:
case 0x00000026:
case 0x00000025:
   strcpy(str,"7.0");
   break;

case 0x00000022:
case 0x00000021:
case 0x00000020:
   strcpy(str,"6.0");
   break;

case 0x0000001E:
case 0x0000001C:
   strcpy(str,"5.7");
   break;

case 0x0000001B:
case 0x00000019:
   strcpy(str,"5.0");
   break;

case 0x00000018:
   strcpy(str,"5.0/4.9");
   break;

case 0x00000017:
case 0x00000016:
   strcpy(str,"4.9");
   break;

case 0x00000015:
case 0x00000014:
   strcpy(str,"4.8");
   break;

case 0x00000011:
   strcpy(str,"4.6");
   break;

case 0x00000010:
case 0x0000000f:
   strcpy(str,"4.5");
   break;

case 0x0000000b:
   strcpy(str,"4.0");
   break;

default:
   sprintf(str,"unknown (0x%X)",version);
}

if ((version&0xf0000000)==0x40000000)
   strcat(str," + has audio");

if ((version&0x0f000000)== 0x04000000)
   strcat(str," + eraomnix");

}

/* EOF */

// vim:ts=3:expandtab

