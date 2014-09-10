/* LICENSE
 * 
 * 
 */
#ifndef HANDLERS_NMAP_H
#define HANDLERS_NMAP_H

#include <arpa/inet.h>

enum nmap_action {
  HOP,
  PORT,
  SERVICE,
  OS
};

enum nmap_proto {
  TCP,
  UDP,
  ICMP,
  IGMP,
  UNKNOWN
};

struct nmap_hop_info {
  char            nmap_action;    ///< must be set to ::HOP
  uint16_t        hop;            ///< the hop number
  uint32_t        usec;           ///< useconds for reach this address
  in_addr_t       address;        ///< the address
};

struct nmap_port_info {
  char            nmap_action;    ///< must be set to ::PORT
  char            proto;          ///< ::nmap_proto used by this port
  uint16_t        port;           ///< the discovered port
};

struct nmap_service_info {
  char            nmap_action;    ///< must be set to ::SERVICE
  char            proto;          ///< ::nmap_proto used by this port
  uint16_t        port;           ///< the port number
  uint16_t        version_offset; ///< offset of the version string in ::nmap_service_info.name
  char            name[];         ///< string containing service name ( and version if ::nmap_service_info.version_offset != 0 )
};

struct nmap_os_info {
  char            nmap_action;    ///< must be set to ::OS
  uint8_t         accuracy;       ///< accuracy of this result
  uint16_t        type_offset;    ///< offset of the device type string in ::nmap_os_info.os
  char            os[];           ///< string containing discovered OS and device type
};

message *nmap_output_parser(char *);

#endif
