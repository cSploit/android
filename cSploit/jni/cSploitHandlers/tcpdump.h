/* LICENSE
 * 
 * 
 */
#ifndef HANDLERS_TCPDUMP_H
#define HANDLERS_TCPDUMP_H

#include <arpa/inet.h>

enum tcpdump_action {
  TCPDUMP_PACKET
};

struct tcpdump_packet_info {
  char          tcpdump_action; ///< must be set to ::TCPDUMP_PACKET
  in_addr_t     src;            ///< source address of the packet
  in_addr_t     dst;            ///< destination address of the packet
  uint16_t      len;            ///< length of the packet
};

message *tcpdump_output_parser(char *);

#endif
