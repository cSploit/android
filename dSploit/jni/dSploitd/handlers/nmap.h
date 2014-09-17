/* LICENSE
 * 
 * 
 */
#ifndef HANDLERS_NMAP_H
#define HANDLERS_NMAP_H

#include <arpa/inet.h>

#define NMAP_HANDLER_ID 2

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
  /**
   * @brief array of strings ( separaed by ::STRING_SEPARATOR )
   * service[0] is the service name
   * service[1] is the verison ( if found )
   */
  char            service[];
};

struct nmap_os_info {
  char            nmap_action;    ///< must be set to ::OS
  uint8_t         accuracy;       ///< accuracy of this result
  /**
   * @brief array of strings ( separaed by ::STRING_SEPARATOR )
   * os[0] is the OS
   * os[1] is the device type
   */
  char            os[];
};

message *nmap_output_parser(char *);

#endif
