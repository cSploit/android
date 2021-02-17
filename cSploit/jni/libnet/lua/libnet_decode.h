#include <libnet.h>

#ifndef LIBNET_DECODE_H
#define LIBNET_DECODE_H

int libnet_decode_tcp(const uint8_t* pkt, size_t pkt_s, libnet_t *l);
int libnet_decode_udp(const uint8_t* pkt, size_t pkt_s, libnet_t *l);
int libnet_decode_ipv4(const uint8_t* pkt, size_t pkt_s, libnet_t *l);
int libnet_decode_ip(const uint8_t* pkt, size_t pkt_s, libnet_t *l);
int libnet_decode_eth(const uint8_t* pkt, size_t pkt_s, libnet_t *l);

#endif

