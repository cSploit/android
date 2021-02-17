/*
    ipxparse.c
    Copyright 1996 Volker Lendecke, Goettingen, Germany
    Copyright 1997, 1998, 1999, 2001 Petr Vandrovec, Prague, Czech Republic

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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


    Revision history:

	0.00  1996-1999			Volker Nendecke, Petr Vandrovec
		Initial revision, different changes.
		
	1.00  2001, July 15		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added #include <stdlib> to disable some warnings.

 */

#include <unistd.h>
#include <sys/types.h>
#include <ncp/ext/socket.h>
/* TBD */
#include <netinet/in.h>
#ifdef HAVE_NETINET_IF_ETHER_H
#include <netinet/if_ether.h>
#else
#include <linux/if_ether.h>
#endif
#include <string.h>
#include <sys/ioctl.h>
#include <ncp/kernel/if.h>
#include <signal.h>
#include <stdlib.h>
/* TBD */
#include <netinet/ip.h>
/* TBD */
#include <netinet/tcp.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>
#include "ipxutil.h"

#define DUMPALLSAPS		/* #define if you want to dump all SAP's */

struct ipx_address
{
	IPXNet	net	__attribute__((packed));
	IPXNode	node	__attribute__((packed));
	IPXPort	sock	__attribute__((packed));
};

struct ipx_packet
{
	unsigned short ipx_checksum	__attribute__((packed));
#define IPX_NO_CHECKSUM	0xFFFF
	unsigned short ipx_pktsize	__attribute__((packed));
	unsigned char ipx_tctrl		__attribute__((packed));
	unsigned char ipx_type		__attribute__((packed));
#define IPX_TYPE_UNKNOWN	0x00
#define IPX_TYPE_RIP		0x01	/* may also be 0 */
#define IPX_TYPE_SAP		0x04	/* may also be 0 */
#define IPX_TYPE_SPX		0x05	/* Not yet implemented */
#define IPX_TYPE_NCP		0x11	/* $lots for docs on this (SPIT) */
#define IPX_TYPE_PPROP		0x14	/* complicated flood fill brdcast [Not supported] */
	struct ipx_address ipx_dest __attribute__((packed));
	struct ipx_address ipx_source __attribute__((packed));
};

#define NCP_ALLOC_SLOT_REQUEST   (0x1111)
#define NCP_REQUEST              (0x2222)
#define NCP_DEALLOC_SLOT_REQUEST (0x5555)

struct ncp_request_header
{
	u_int16_t type __attribute__((packed));
	u_int8_t sequence __attribute__((packed));
	u_int8_t conn_low __attribute__((packed));
	u_int8_t task __attribute__((packed));
	u_int8_t conn_high __attribute__((packed));
	u_int8_t function __attribute__((packed));
	u_int8_t data[0] __attribute__((packed));
};

#define NCP_REPLY                (0x3333)
#define NCP_POSITIVE_ACK         (0x9999)

struct ncp_reply_header
{
	u_int16_t type __attribute__((packed));
	u_int8_t sequence __attribute__((packed));
	u_int8_t conn_low __attribute__((packed));
	u_int8_t task __attribute__((packed));
	u_int8_t conn_high __attribute__((packed));
	u_int8_t completion_code __attribute__((packed));
	u_int8_t connection_state __attribute__((packed));
	u_int8_t data[0] __attribute__((packed));
};

#define NCP_BURST_PACKET         (0x7777)

struct ncp_burst_header
{
	u_int16_t type __attribute__((packed));
	u_int8_t system_flags __attribute__((packed));
	u_int8_t stream_type __attribute__((packed));
	u_int32_t source_conn __attribute__((packed));
	u_int32_t dest_conn __attribute__((packed));
	u_int32_t packet_sequence __attribute__((packed));
	u_int32_t send_delay __attribute__((packed));
	u_int16_t burst_sequence __attribute__((packed));
	u_int16_t ack_sequence __attribute__((packed));
	u_int32_t burst_length __attribute__((packed));
	u_int32_t data_offset __attribute__((packed));
	u_int16_t data_bytes __attribute__((packed));
	u_int16_t missing_frags __attribute__((packed));
};

void handle_ipx(unsigned char *buf, int length, char *frame, int no);
void handle_ncp(struct sockaddr_ipx *source,
		struct sockaddr_ipx *target,
		unsigned char *buf, int length, int no);
int handle_burst(struct sockaddr_ipx *source,
		 struct sockaddr_ipx *target,
		 unsigned char *buf, int length, int no);

#define	SAP_MAX_SERVER_NAME_LENGTH	48	/* in network packets */
#define	SAP_MAX_SAPS_PER_PACKET		7
#define	SAP_SHUTDOWN	16	/* Magic "hops" value to stop SAP advertising */

/* SAP Query structure (returned in sap_packet as an array)
 * NBO == Network Byte Order)
 */
typedef struct saps
{
	u_int16_t serverType __attribute__((packed));	/* NBO */
	u_int8_t serverName[SAP_MAX_SERVER_NAME_LENGTH] __attribute__((packed));
	struct ipx_address serverAddress __attribute__((packed));
	u_int16_t serverHops __attribute__((packed));	/* NBO */
}
SAPS;

/* General Service/Nearest Server Response SAP packet */
union sap_packet
{
	unsigned short sapOperation;
	struct sap_query
	{
		u_int16_t sapOperation __attribute__((packed));
		u_int16_t serverType __attribute__((packed));
	}
	query;
	struct sap_response
	{
		u_int16_t sapOperation __attribute__((packed));
		/* each SAP can has a max of SAP_MAX_SAPS_PER_PACKET packets */
		SAPS sap[SAP_MAX_SAPS_PER_PACKET] __attribute__((packed));
	}
	response;
};

/* print out one SAP record */
static void
print_sap(FILE * file, SAPS * sapp)
{
	fprintf(file, "  Name:%s, serverType 0x%x, ",
		sapp->serverName,
		ntohs(sapp->serverType));
	ipx_fprint_network(file, ntohl(sapp->serverAddress.net));
	fprintf(file, ":");
	ipx_fprint_node(file, sapp->serverAddress.node);
	fprintf(file, ":");
	ipx_fprint_port(file, ntohs(sapp->serverAddress.sock));
	fprintf(file, " (Hops %d)\n", ntohs(sapp->serverHops));
}

void
handle_ipx(unsigned char *buf, int length, char *frame, int no)
{
	struct ipx_packet *h;
	struct sockaddr_ipx s_addr;
	struct sockaddr_ipx d_addr;
	union sap_packet *sappacket;
	int hbo_dsock;		/* Host Byte Order of Destination SOCKet */
	int hbo_sapop;		/* Host Byte Order of SAP OPeration */

	if (frame && !strcmp(frame, "raweth")) {
		int skip;
		
		if (((buf[12] << 8) | buf[13]) <= 0x5DC) {
			skip = (buf[14] << 8) | buf[15];
			switch (skip) {
				case 0xFFFF: skip = 12; break;
				case 0xAAAA: skip = 12 + 3 + 5; break;
				default:     skip = 12 + 3; break;
			}
		} else
			skip = 12 + 2;
		if (length < skip)
			return;	/* Whoops! */
		length -= skip;
		buf += skip;
	}
	h = (struct ipx_packet *)buf;
	memset(&s_addr, 0, sizeof(s_addr));
	memset(&d_addr, 0, sizeof(d_addr));

	memcpy(s_addr.sipx_node, h->ipx_source.node, sizeof(s_addr.sipx_node));
	s_addr.sipx_port = h->ipx_source.sock;
	s_addr.sipx_network = h->ipx_source.net;

	memcpy(d_addr.sipx_node, h->ipx_dest.node, sizeof(d_addr.sipx_node));
	d_addr.sipx_port = h->ipx_dest.sock;
	d_addr.sipx_network = h->ipx_dest.net;

	printf("%6.6d %s from ", no, frame);

	ipx_print_saddr(&s_addr);
	printf(" to ");
	ipx_print_saddr(&d_addr);
	printf("\n");

	if (handle_burst(&s_addr, &d_addr, buf + sizeof(struct ipx_packet),
			 length - sizeof(struct ipx_packet), no) != 0)
	{
		return;
	}
	if ((ntohs(s_addr.sipx_port) == 0x451)
	    || (ntohs(d_addr.sipx_port) == 0x451))
	{
		handle_ncp(&s_addr, &d_addr, buf + sizeof(struct ipx_packet),
			   length - sizeof(struct ipx_packet), no);
	} else
		/* next 3 handle IPX by type vs by socket (one or other) */
		/* Note: most things use either ipx_type OR socket, not both */ if (h->ipx_type == 0x01)
		printf(" type 0x01 (RIP packet (router))\n");
	else if (h->ipx_type == 0x05)
		printf(" type 0x05 (SPX sequenced packet)\n");
	else if (h->ipx_type == 0x14)
		printf(" type 0x14 (propogated Client-NetBios)\n");
	else
	{
		hbo_dsock = ntohs(d_addr.sipx_port);
		if (hbo_dsock == 0x452)		/* SAP */
		{
			sappacket = (union sap_packet *)
			    (buf + sizeof(struct ipx_packet));
			hbo_sapop = ntohs(sappacket->sapOperation);
			if ((hbo_sapop == 0x01) || (hbo_sapop == 0x03))
			{
				printf(" type 0x%x, SAP op:0x%x %s Query, "
				       "serverType 0x%x wanted\n",
				       h->ipx_type, hbo_sapop,
				     (hbo_sapop == 0x01) ? "General Service" :
				       (hbo_sapop == 0x03) ? "Nearest Server" :
				       "Error",
				       ntohs(sappacket->query.serverType));
			} else
			{
				int hops;

				hops = ntohs(sappacket->
					     response.sap[0].serverHops);
				printf(" type 0x%x, SAP op:0x%x %s %s\n",
				       h->ipx_type, hbo_sapop,
				       (hbo_sapop == 0x02)
				       ? "General Service Response" :
				       (hbo_sapop == 0x04)
				       ? "Nearest Server Response" :
				       "Unknown",
				       (hops >= SAP_SHUTDOWN)
				       ? "[Shutdown]" : "");

				/* Service ending */
				if (hops >= SAP_SHUTDOWN)
				{
					print_sap(stdout,
						  sappacket->response.sap);
				}
#ifdef DUMPALLSAPS
				/* If you want to dump all SAP's */
				else
				{
					int num_saps;
					SAPS *sapp;

					num_saps = (length
						    - sizeof(struct ipx_packet)
						    - 2) / sizeof(SAPS);

					sapp = sappacket->response.sap;
					for (; num_saps > 0; sapp++, num_saps--)
						print_sap(stdout, sapp);
				}
#endif				/* DUMPALLSAPS */
			}

		} else		/* Other IPX types */
			printf(" type 0x%x, Socket 0x%x (%s)\n", h->ipx_type,
			       hbo_dsock,
			       (hbo_dsock == 0x451) ? "NCP" :
/*                     (hbo_dsock == 0x452) ? "SAP" : */
			       (hbo_dsock == 0x453) ? "RIP" :
			       (hbo_dsock == 0x455) ? "Client-NetBios" :
			       (hbo_dsock == 0x456) ? "Diags" :
			       (hbo_dsock == 0x002) ? "Xecho" :
			       (hbo_dsock == 0x8063) ? "NVT2" : "Other");
	}

}

int
handle_burst(struct sockaddr_ipx *source,
	     struct sockaddr_ipx *target,
	     unsigned char *buf, int length, int no)
{
	struct ncp_burst_header *rq = (struct ncp_burst_header *) buf;

	if (rq->type != NCP_BURST_PACKET)
	{
		return 0;
	}
	printf("Burst Packet\n");
	printf("Stream Type: %02X, System Flags: %02X\n",
	       rq->stream_type, rq->system_flags);
	printf("Source Conn: %08X, Dest Conn: %08X, Packet Seq: %08X\n",
	       rq->source_conn, rq->dest_conn,
	       (unsigned int) ntohl(rq->packet_sequence));
	printf("Send Delay:  %08X, Burst Seq: %04X, Ack Seq: %04X\n",
	       (unsigned int) ntohl(rq->send_delay), ntohs(rq->burst_sequence),
	       ntohs(rq->ack_sequence));
	printf("Burst Length: %08X\n", (unsigned int) ntohl(rq->burst_length));
	printf("Data Offset: %08X, Data Bytes: %04X, Missing Frags: %04X\n",
	       (unsigned int) ntohl(rq->data_offset), ntohs(rq->data_bytes),
	       ntohs(rq->missing_frags));

	if (ntohs(rq->data_bytes) == 24)
	{
		struct ncp_burst_request
		{
			struct ncp_burst_header h __attribute__((packed));
			u_int32_t function __attribute__((packed));
			u_int32_t file_handle __attribute__((packed));
			u_int8_t reserved[8] __attribute__((packed));
			u_int32_t file_offset __attribute__((packed));
			u_int32_t number_of_bytes __attribute__((packed));
		}
		*brq = (struct ncp_burst_request *) rq;

		printf("Assuming Burst Request:\n");
		printf("%s: Handle %08X, Offset %08X, Bytes %08X\n",
		       brq->function == 1 ? "Read " : "Write",
		       brq->file_handle,
		       (unsigned int) ntohl(brq->file_offset),
		       (unsigned int) ntohl(brq->number_of_bytes));
	}
	printf("\n");
	return 1;
}

void
handle_ncp(struct sockaddr_ipx *source,
	   struct sockaddr_ipx *target,
	   unsigned char *buf, int length, int no)
{
	struct ncp_request_header *rq = (struct ncp_request_header *) buf;
	struct ncp_reply_header *rs = (struct ncp_reply_header *) buf;
	unsigned char *data = NULL;
	int data_length = 0;
	int i;

	static struct sockaddr_ipx request_source;
	static struct ncp_request_header request_header;
	static char request_data[5];

	if (ntohs(rq->type) == NCP_REQUEST)
	{
		/* Request */
		printf("NCP request: conn: %-5d, seq: %-3d, task: %-3d, ",
		       rq->conn_low + 256 * rq->conn_high,
		       rq->sequence, rq->task);

		memcpy(&request_source, source, sizeof(request_source));
		memcpy(&request_header, rq, sizeof(request_header));
		memcpy(request_data, rq + 1, sizeof(request_data));

		data = buf + sizeof(struct ncp_request_header);
		data_length = length - sizeof(struct ncp_request_header);

		switch (rq->function)
		{
		case 18:
			printf("fn: %-3d\n", rq->function);
			printf("Get Volume Info with Number\n");
			break;
		case 20:
			printf("fn: %-3d\n", rq->function);
			printf("Get File Server Date and Time\n");
			break;
		case 21:
			printf("fn: %-3d, subfn: %-3d\n",
			       rq->function, data[2]);
			switch (data[2])
			{
			case 0:
				printf("Send Broadcast Message\n");
				break;
			case 1:
				printf("Get Broadcast Message\n");
				break;
			}
			data += 3;
			data_length -= 3;
			break;
		case 22:
			printf("fn: %-3d, subfn: %-3d\n",
			       rq->function, data[2]);
			switch (data[2])
			{
			case 00:
				printf("Set Directory Handle\n");
				break;
			case 01:
				printf("Get Directory Path\n");
				break;
			case 02:
				printf("Scan Directory Information\n");
				break;
			case 03:
				printf("Get Effective Directory Rights\n");
				break;
			case 05:
				printf("Get Volume Number\n");
				break;
			case 06:
				printf("Get Volume Name\n");
				break;
			case 10:
				printf("Create directory\n");
				break;
			case 18:
				printf("Allocate Permanent Dir Handle\n");
				break;
			case 20:
				printf("Deallocate Directory Handle\n");
				break;
			case 21:
				printf("Get Volume Info with handle\n");
				break;
			case 39:
				printf("Add ext. Trustee to Dir or File\n");
				break;
			case 48:
				printf("Get Name Space Directory Entry\n");
				break;
			}
			data += 3;
			data_length -= 3;
			break;
		case 23:
			printf("fn: %-3d, subfn: %-3d\n", rq->function,
			       data[2]);
			switch (data[2])
			{
			case 17:
				printf("Get Fileserver Information\n");
				break;
			case 22:
				printf("Get Station's logged Info (old)\n");
				break;
			case 23:
				printf("Get Crypt Key\n");
				break;
			case 24:
				printf("Encrypted Login\n");
				break;
			case 28:
				printf("Get Connection Information\n");
				break;
			case 50:
				printf("Create Bindery Object\n");
				break;
			case 53:
				printf("Get Bindery Object ID\n");
				break;
			case 54:
				printf("Get Bindery Object Name\n");
				break;
			case 55:
				printf("Scan Bindery Object\n");
				break;
			case 57:
				printf("Create Property\n");
				break;
			case 59:
				printf("Change Property Security\n");
				break;
			case 60:
				printf("Scan Property\n");
				break;
			case 61:
				printf("Read Property Value\n");
				break;
			case 62:
				printf("Write Property Value\n");
				break;
			case 65:
				printf("Add Bindery Object to Set\n");
				break;
			case 67:
				printf("Is Bindery Object in Set\n");
				break;
			case 70:
				printf("Get Bindery Access Level\n");
				break;
			case 72:
				printf("Get Bindery Object Access Level\n");
				break;
			case 75:
				printf("Keyed change password\n");
				break;
			case 113:
				printf("Service Queue Job (old)\n");
				break;
			case 124:
				printf("Service Queue Job \n");
				break;
			}

			data += 3;
			data_length -= 3;
			break;
		case 24:
			printf("fn: %-3d\n", rq->function);
			printf("End of Job\n");
			break;
		case 33:
			printf("fn: %-3d\n", rq->function);
			printf("Negotiate Buffer size\n");
			break;
		case 34:
			printf("fn: %-3d, subfn: %-3d\n", rq->function,
			       data[2]);
			data += 3;
			data_length -= 3;
			break;
		case 62:
			printf("fn: %-3d\n", rq->function);
			printf("File Search Initialize\n");
			break;
		case 63:
			printf("fn: %-3d\n", rq->function);
			printf("File Search Continue\n");
			break;
		case 64:
			printf("fn: %-3d\n", rq->function);
			printf("Search for a file\n");
			break;
		case 66:
			printf("fn: %-3d\n", rq->function);
			printf("Close File\n");
			break;
		case 67:
			printf("fn: %-3d\n", rq->function);
			printf("Create File\n");
			break;
		case 72:
			printf("fn: %-3d\n", rq->function);
			printf("Read from File\n");
			break;
		case 73:
			printf("fn: %-3d\n", rq->function);
			printf("Write to File\n");
			break;
		case 75:
			printf("fn: %-3d\n", rq->function);
			printf("Set File Time Date Stamp\n");
			break;
		case 76:
			printf("fn: %-3d\n", rq->function);
			printf("Open File (old)\n");
			break;

		case 87:
			printf("fn: %-3d, subfn: %-3d\n",
			       rq->function, data[0]);
			switch (data[0])
			{
			case 1:
				{
					unsigned char *p = &(data[0]);
					printf("Open Create File or Subdirectory\n");
					printf("Name Space: %d\n", p[1]);
					printf("Open Create Mode: %x\n", p[2]);
					printf("Search Attributes: %x\n",
					       *(u_int16_t *) & (p[3]));
					printf("Return Information Mask: %x\n",
					(unsigned int) (*(u_int32_t *) & (p[5])));
					printf("Desired Access Rights: %x\n",
					       *(u_int16_t *) & (p[9]));
					break;
				}
			case 2:
				printf("Initialize Search\n");
				break;
			case 3:
				printf("Search for File or Subdirectory\n");
				break;
			case 6:
				printf("Obtain File Or Subdirectory "
				       "Information\n");
				break;
			case 8:
				printf("Delete a File Or Subdirectory\n");
				break;
			case 12:
				printf("Allocate Short Directory Handle\n");
				break;
			}
			data += 1;
			data_length -= 1;
			break;
		case 97:
			{
				struct INPUT
				{
					u_int16_t proposed_max_size;
					u_int8_t security_flag;
				}
				*i = (struct INPUT *) data;

				printf("fn: %-3d\n", rq->function);
				printf("Get Big Packet NCP Max Packet Size\n");
				printf("proposed_max_size: %x\n",
				       ntohs(i->proposed_max_size));
				printf("security_flag: %x\n",
				       i->security_flag);
				break;
			}
		case 101:
			{
				struct INPUT
				{
					u_int32_t local_conn_id;
					u_int32_t local_max_packet_size;
					u_int16_t local_target_socket;
					u_int32_t local_max_send_size;
					u_int32_t local_max_recv_size;
				}
				*i = (struct INPUT *) data;

				printf("fn: %-3d\n", rq->function);
				printf("Packet Burst Connection Request\n");
				printf("local_conn_id: %x\n",
				       (u_int32_t)ntohl(i->local_conn_id));
				printf("local_max_packet_size: %x\n",
				       (u_int32_t)ntohl(i->local_max_packet_size));
				printf("local_target_socket: %x\n",
				       (u_int32_t)ntohl(i->local_target_socket));
				printf("local_max_send_size: %x\n",
				       (u_int32_t)ntohl(i->local_max_send_size));
				printf("local_max_recv_size: %x\n",
				       (u_int32_t)ntohl(i->local_max_recv_size));
			}
			break;
		case 104:
			{
				printf("fn: %-3d, subfn: %-3d, NDS call\n",
				       rq->function, data[0]);

				/* I took this information from the (german!!)
				   book 'Einf"uhrung in die NetWare LAN
				   Analyse', Laura A. Chappell, Dan E. Hakes,
				   Novell Press, Markt & Technik, ISBN
				   3-8272-5084-6, and from the book mentioned
				   in the ncpfs README. I'm not sure it is
				   correct, because I do not have NW 4.x. If
				   you have the time, could you watch a NW4
				   workstation, and tell me whether anything
				   of this makes sense at all. */

				switch (data[0])
				{
				case 1:
					printf("Ping for NDS\n");
					break;
				case 2:
					{
						struct INPUT
						{
							u_int8_t subfunction_code;
							u_int32_t fragger_handle;
							u_int32_t max_fragment_size;
							u_int32_t message_size;
							u_int32_t fragment_flag;
							u_int32_t verb;
						}
						*i = (struct INPUT *) data;
						printf("Send NDS Fragment Request/Reply\n");
						printf("fragger_handle: %lx\n",
						       (unsigned long) i->fragger_handle);
						printf("max_fragment_size: %lx\n",
						       (unsigned long) i->max_fragment_size);
						printf("message_size: %lx\n",
						       (unsigned long) i->message_size);
						printf("fragment_flag: %lx\n",
						       (unsigned long) i->fragment_flag);
						printf("verb: %d\n", i->verb);

						switch (i->verb)
						{
						case 1:
							printf("Resolve Name\n");
							break;
						case 2:
							printf("Read Entry Information\n");
							break;
						case 3:
							printf("Read\n");
							break;
						case 4:
							printf("Compare\n");
							break;
						case 5:
							printf("List\n");
							break;
						case 6:
							printf("Search Entries\n");
							break;
						case 7:
							printf("Add Entry\n");
							break;
						case 8:
							printf("Remove Entry\n");
							break;
						case 9:
							printf("Modify Entry\n");
							break;
						case 10:
							printf("Modify RDN\n");
							break;
						case 11:
							printf("Create Attribute\n");
							break;
						case 12:
							printf("Read Attribute Definition\n");
							break;
						case 13:
							printf("Remove Attribute Definition\n");
							break;
						case 14:
							printf("Define Class\n");
							break;
						case 15:
							printf("Read Class Definition\n");
							break;
						case 16:
							printf("Modify Class Definition\n");
							break;
						case 17:
							printf("Remove Class Definition\n");
							break;
						case 18:
							printf("List Containable Classes\n");
							break;
						case 19:
							printf("Get Effective Rights\n");
							break;
						case 20:
							printf("Add Partition\n");
							break;
						case 21:
							printf("Remove Partition\n");
							break;
						case 22:
							printf("List Partitions\n");
							break;
						case 23:
							printf("Split Partition\n");
							break;
						case 24:
							printf("Join Partitions\n");
							break;
						case 25:
							printf("Add Replica\n");
							break;
						case 26:
							printf("Remove Replica\n");
							break;
						case 27:
							printf("Open Stream\n");
							break;
						case 28:
							printf("Search Filter\n");
							break;
						case 29:
							printf("Create Subordinate Reference\n");
							break;
						case 30:
							printf("Link Replica\n");
							break;
						case 31:
							printf("Change Replica Type\n");
							break;
						case 32:
							printf("Start Update Schema\n");
							break;
						case 33:
							printf("End Update Schema\n");
							break;
						case 34:
							printf("Update Schema\n");
							break;
						case 35:
							printf("Start Update Replica\n");
							break;
						case 36:
							printf("End Update Replica\n");
							break;
						case 37:
							printf("Update Replica\n");
							break;
						case 38:
							printf("Synchronize Partition\n");
							break;
						case 39:
							printf("Synchronize Schema\n");
							break;
						case 40:
							printf("Read Syntaxes\n");
							break;
						case 41:
							printf("Get Replica Root ID\n");
							break;
						case 42:
							printf("Begin Move Entry\n");
							break;
						case 43:
							printf("Finish Move Entry\n");
							break;
						case 44:
							printf("Release Moved Entry\n");
							break;
						case 45:
							printf("Backup Entry\n");
							break;
						case 46:
							printf("Restore Entry\n");
							break;
						case 47:
							printf("Save DIB\n");
							break;
						case 48:
						case 49:
							printf("Unused\n");
							break;
						case 50:
							printf("Close Iteration\n");
							break;
						case 51:
							printf("Unused\n");
							break;
						case 52:
							printf("Audit Skulking\n");
							break;
						case 53:
							printf("Get Server Address\n");
							break;
						case 54:
							printf("Set Keys\n");
							break;
						case 55:
							printf("Change Password\n");
							break;
						case 56:
							printf("Verify Password\n");
							break;
						case 57:
							printf("Begin Login\n");
							break;
						case 58:
							printf("Finish Login\n");
							break;
						case 59:
							printf("Begin Authentication\n");
							break;
						case 60:
							printf("Finish Authentication\n");
							break;
						case 61:
							printf("Logout\n");
							break;
						case 62:
							printf("Repair Ring\n");
							break;
						case 63:
							printf("Repair Timestamps\n");
							break;
						case 64:
							printf("Create Back Link\n");
							break;
						case 65:
							printf("Delete External Reference\n");
							break;
						case 66:
							printf("Rename External Reference\n");
							break;
						case 67:
							printf("Create Directory Entry\n");
							break;
						case 68:
							printf("Remove Directory Entry\n");
							break;
						case 69:
							printf("Designate New Master\n");
							break;
						case 70:
							printf("Change Tree Name\n");
							break;
						case 71:
							printf("Partition Entry Count\n");
							break;
						case 72:
							printf("Check Login Restrictions\n");
							break;
						case 73:
							printf("Start Join\n");
							break;
						case 74:
							printf("Low Level Split\n");
							break;
						case 75:
							printf("Low Level Join\n");
							break;
						case 76:
							printf("Abort Low Level Join\n");
							break;
						case 77:
							printf("Get All Servers\n");
							break;
						default:
							printf("Unknown Verb: %d\n",
							       data[0]);
							break;
						}
						break;
					}
				case 3:
					printf("Close NDS Fragment\n");
					break;
				case 4:
					printf("Return Bindery Context\n");
					break;
				case 5:
					printf("Monitor NDS connection\n");
					break;
				case 200:
					printf("NDS Auditing\n");
					break;
				default:
					break;
				}
			}
		default:
			printf("fn: %-3d\n", rq->function);
		}
	}
	if (ntohs(rs->type) == NCP_REPLY)
	{
		printf("NCP respons: conn: %-5d, seq: %-3d, task: %-3d, ",
		       rs->conn_low + 256 * rs->conn_high,
		       rs->sequence, rs->task);
		printf("compl: %-3d, conn_st: %-3d\n",
		       rs->completion_code, rs->connection_state);

		data = buf + sizeof(struct ncp_reply_header);
		data_length = length - sizeof(struct ncp_reply_header);

		if ((memcmp(&request_source, target,
			    sizeof(request_source)) == 0)
		    && (request_header.sequence == rs->sequence))
		{
			switch (request_header.function)
			{
			case 22:
				switch (request_data[2])
				{
				case 18:
					{
						struct XDATA
						{
							u_int8_t new_directory_handle;
							u_int8_t access_rights_mask;
						}
						*x = (struct XDATA *) data;
						printf("new_directory_handle: %x\n",
						     x->new_directory_handle);
						printf("access_rights_mask: %x\n",
						       x->access_rights_mask);
					}
					break;
				}
				break;
			case 72:
				printf("Read data\n");
				data_length = 0;
				break;
			case 97:
				{
					struct XDATA
					{
						u_int16_t accepted_max_size;
						u_int16_t echo_socket;
						u_int8_t security_flag;
					}
					*x = (struct XDATA *) data;
					printf("accepted_max_size: %x\n",
					       ntohs(x->accepted_max_size));
					printf("echo_socket: %x\n",
					       ntohs(x->echo_socket));
					printf("security_flag: %x\n",
					       (x->security_flag));
				}
				break;
			case 101:
				{
					struct XDATA
					{
						u_int8_t completion_code;
						u_int32_t remote_target_id;
						u_int32_t remote_max_packet_size;
					}
					*x = (struct XDATA *) data;
					printf("completion_code: %x\n",
					       x->completion_code);
					printf("remote_target_id: %x\n",
					       (u_int32_t)ntohl(x->remote_target_id));
					printf("remote_max_packet_size: %x\n",
					       (u_int32_t)ntohl(x->remote_max_packet_size));
				}
				break;
			}
		}
	}
	if (data == NULL)
	{
		data = buf;
		data_length = length;
	}
	i = 0;
	while (i < data_length)
	{
		int j;
		for (j = i; j < i + 16; j++)
		{
			if (j >= data_length)
			{
				printf("  ");
			} else
			{
				printf("%-2.2X", data[j]);
			}
		}
		printf(" ");
		for (j = i; j < i + 16; j++)
		{
			if (j >= data_length)
			{
				break;
			}
			if (isprint(data[j]))
			{
				printf("%c", data[j]);
			} else
			{
				printf(".");
			}
		}
		printf("\n");
		i += 16;
	}
	printf("\n");
}


int
main(int argc, char *argv[])
{
	unsigned char buf[16384];
	unsigned char packet[8192];
	unsigned char *b;
	int len;
	int i = 1;

	while (fgets(buf, sizeof(buf), stdin) != NULL)
	{
		if (strlen(buf) == sizeof(buf) - 1)
		{
			fprintf(stderr, "line too long\n");
			exit(1);
		}
		b = strchr(buf, ' ');
		if (b == NULL)
		{
			fprintf(stderr, "illegal line format\n");
			exit(1);
		}
		*b = '\0';
		b += 1;
		len = 0;

		while ((b[0] != '\0') && (b[1] != '\0'))
		{
			unsigned int value;
			if (sscanf(b, "%2x", &value) != 1)
			{
				fprintf(stderr, "illegal packet\n");
				exit(1);
			}
			packet[len] = value;
			b += 2;
			len += 1;
		}
		handle_ipx(packet, len, buf, i);
		i += 1;
	}
	return 0;
}
