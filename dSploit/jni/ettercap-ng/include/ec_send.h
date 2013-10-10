
/* $Id: ec_send.h,v 1.19 2004/09/02 13:17:58 alor Exp $ */

#ifndef EC_SEND_H
#define EC_SEND_H

#include <ec_packet.h>
#include <pcap.h>
#include <libnet.h>

EC_API_EXTERN void send_init(void);
EC_API_EXTERN int send_to_L2(struct packet_object *po);
EC_API_EXTERN int send_to_L3(struct packet_object *po);
EC_API_EXTERN int send_to_bridge(struct packet_object *po);

EC_API_EXTERN int send_arp(u_char type, struct ip_addr *sip, u_int8 *smac, struct ip_addr *tip, u_int8 *tmac);
EC_API_EXTERN int send_L2_icmp_echo(u_char type, struct ip_addr *sip, struct ip_addr *tip, u_int8 *tmac);
EC_API_EXTERN int send_L3_icmp_echo(u_char type, struct ip_addr *sip, struct ip_addr *tip);
EC_API_EXTERN int send_icmp_redir(u_char type, struct ip_addr *sip, struct ip_addr *gw, struct packet_object *po);
EC_API_EXTERN int send_dhcp_reply(struct ip_addr *sip, struct ip_addr *tip, u_int8 *tmac, u_int8 *dhcp_hdr, u_int8 *options, size_t optlen);
EC_API_EXTERN int send_dns_reply(u_int16 dport, struct ip_addr *sip, struct ip_addr *tip, u_int8 *tmac, u_int16 id, u_int8 *data, size_t datalen, u_int16 addi_rr);
EC_API_EXTERN int send_tcp(struct ip_addr *sip, struct ip_addr *tip, u_int16 sport, u_int16 dport, u_int32 seq, u_int32 ack, u_int8 flags);
EC_API_EXTERN int send_tcp_ether(u_int8 *dmac, struct ip_addr *sip, struct ip_addr *tip, u_int16 sport, u_int16 dport, u_int32 seq, u_int32 ack, u_int8 flags);
EC_API_EXTERN int send_L3_icmp_unreach(struct packet_object *po);

EC_API_EXTERN void capture_only_incoming(pcap_t *p, libnet_t *l);

EC_API_EXTERN u_int8 MEDIA_BROADCAST[MEDIA_ADDR_LEN];
EC_API_EXTERN u_int8 ARP_BROADCAST[MEDIA_ADDR_LEN];

#define FUNC_BUILDER(func)       libnet_ptag_t func(u_int8 *dst, u_int16 proto)
#define FUNC_BUILDER_PTR(func)   libnet_ptag_t (*func)(u_int8 *dst, u_int16 proto)

EC_API_EXTERN void add_builder(u_int8 dlt, FUNC_BUILDER_PTR(builder));

#endif

/* EOF */

// vim:ts=3:expandtab

