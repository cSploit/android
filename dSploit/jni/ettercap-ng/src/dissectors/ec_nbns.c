/*
    ettercap -- dissector NBNS -- UDP 137 

    Copyright (C) Ettercap Team 

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

/*
 * NBNS RFC: http://www.faqs.org/rfcs/rfc1002.html
 */


#define TYPE_NB 0x0020
#define TYPE_NBSTAT 0x0021
#define CLASS_IN 0x0001

#define NBNS_NAME_LEN 32
#define NBNS_DECODED_NAME_LEN 16

struct nbns_header {
        u_int16 transactid;     /* Transaction ID */
#ifdef WORDS_BIGENDIAN
        u_char  response: 1;    /* response or query */
        u_char  opcode: 4;      /* opcode */
        /* nm_flags */
        u_char  aa: 1;
        u_char  tc: 1;
        u_char  rd: 1;
        u_char  ra: 1;
        u_char  unused: 2;
        u_char  broadcast: 1;
        u_char  rcode: 4;      /* RCODE */
#else
	u_char rd: 1;
	u_char tc: 1;
	u_char aa: 1;
	u_char opcode: 4;
	u_char response: 1;
	u_char rcode: 4;
	u_char broadcast: 1;
	u_char unused: 2;
	u_char ra: 1;
#endif
        u_int16 qd_count;       /* QDCOUNT */
        u_int16 an_count;       /* AN_COUNT */
        u_int16 ns_count;       /* NS_COUNT */
        u_int16 ar_count;       /* AR_COUNT */
};

struct nbns_query {
        struct nbns_header header;
        char question[NBNS_NAME_LEN+2];
	u_int16 type;
	u_int16 class;
};

struct nbns_rdata {
  u_int16 len;
  u_int16 nbflags;
  u_int32 addr;
};

#define NBNS_MSGLEN_QUERY_RESPONSE 70
#define NBNS_TTL_POS 12+1+NBNS_NAME_LEN+1+2+2
#define NBNS_RDATA_POS NBNS_TTL_POS + 2
#define NBNS_QUERY_POS 12

struct nbns_response {
        struct nbns_header header;
        char rr_name[NBNS_NAME_LEN+2];       /* RR_NAME */
        u_int16 type;
        u_int16 class;
        u_int32 ttl;
	struct nbns_rdata rr_data;
};


struct nbns_spoof_entry {
        char *name;
        struct ip_addr ip; /* no ipv6 nbns */
        SLIST_ENTRY(nbns_spoof_entry) next;
};

/* protos */

FUNC_DECODER(dissector_nbns);
void nbns_init(void);
static int nbns_expand(char *compressed, char *dst);


/* 
 * initializer
 */
void __init nbns_init(void)
{
	dissect_add("nbns", APP_LAYER_UDP, 137, dissector_nbns);
}

FUNC_DECODER(dissector_nbns)
{
	struct nbns_header *header;
	struct nbns_query  *query;
	struct nbns_response *response;

	char name[NBNS_NAME_LEN];
	char ip[IP_ASCII_ADDR_LEN];

	memset(name, 0, NBNS_NAME_LEN);

	header = (struct nbns_header *)po->DATA.data;

	DEBUG_MSG("NBNS dissector -> port 137");

	/* HOOK_POINT: HOOK_PROTO_NBNS */
	hook_point(HOOK_PROTO_NBNS, PACKET);


	if (header->response) {
		response = (struct nbns_response *)po->DATA.data;	
		
		if (response->class != CLASS_IN) 
			return NULL;
	
		nbns_expand(response->rr_name, name);

		struct ip_addr addr;
		ip_addr_init(&addr, AF_INET, (u_char*)&response->rr_data.addr);
		ip_addr_ntoa(&addr, ip);
		
		DEBUG_MSG("NBNS Transaction ID [0x%04x] Response: %s -> %s\n", ntohs(header->transactid), name, ip);
	} else {
		query = (struct nbns_query *)po->DATA.data;
		nbns_expand(query->question, name);
		
		DEBUG_MSG("NBNS Transaction ID [0x%04x] Query: %s", ntohs(header->transactid), name);
	}

	return NULL;
}

static int nbns_expand(char *compressed, char *dst)
{
        //format <space>compressed\00
        int len = 0;
	int x=0;	
	char j, k;


	for(len = 1; x < NBNS_NAME_LEN; len+=2){
		j = (compressed[len] & 0x3f)-1;
		k = (compressed[len+1] & 0x3f)-1;	
		dst[x/2] = (j<<4)+(k);
		x+=2;
	}

	char *s = strstr(dst, " ");

	if (s)
		*s = '\0';

        return len;

}

